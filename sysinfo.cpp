#include "sysinfo.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>
#include <QVector>
#include <QThread>
#include <QDebug>
#include "../sysinfo_api/exception.h"
#include "../sysinfo_api/packet.h"

namespace Client
{
Sysinfo::Sysinfo() :
    m_memLoad(0.0),
    m_cpuLoad(0.0)
{
}

API::Packet Sysinfo::makePacket(int clientId) const
{
    API::Packet retPacket = m_packet;
    retPacket.setId(clientId);
    return retPacket;
}

void Sysinfo::loadMeminfo()
{
    m_memLoad = 0.0;
    QFile meminfo("/proc/meminfo");
    if (!meminfo.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        API::Exception("Did not find meminfo file").raise();
    }

    QTextStream parser(&meminfo);
    int memTotal = -1, memFree = -1;
    // We can't use parser.atEnd() here, since it always returns true. This happens with
    // some special files (which include /proc/ files) according to the documentation.
    int retryCount = 0;
    while (memFree < 0 || memTotal < 0)
    {
        QString line = parser.readLine();
        QTextStream lineParser(&line);

        QString mem_type, value, unit;
        lineParser >> mem_type >> value >> unit;
        qDebug() << "meminfo line: " << mem_type << value << unit;
        try
        {
            if (mem_type.startsWith("MemTotal"))
            {
                memTotal = toKb(value, unit);
                qInfo() << "Got" << mem_type << memTotal;
            }
            else if (mem_type.startsWith("MemFree"))
            {
                memFree = toKb(value, unit);
                qInfo() << "Got" << mem_type << memFree;
            }
        }
        catch (API::Exception& except)
        {
            except.append(" in memory info load");
            throw;
        }
        // Usually, those values sit at the top of the file. But we'll use a big value
        // here for garantee.
        if (++retryCount > 100)
        {
            API::Exception("Did not find memory information in meminfo file.").raise();
        }
    }
    if (memFree < 0 && memTotal < 0)
    {
        API::Exception("Invalid formatting in meminfo file: total and free memory not found").raise();
    }
    if (memFree < 0)
    {
        API::Exception("Invalid formatting in meminfo file: free memory not found").raise();
    }
    if (memTotal < 0)
    {
        API::Exception("Invalid formatting in meminfo file: total memory not found").raise();
    }
    if (memFree > memTotal)
    {
        API::Exception("Invalid formatting in meminfo file: free memory bigger than total memory").raise();
    }
    m_memLoad = (double)memFree / memTotal * 100.0;
    qInfo() << "Memory data loaded successfuly";
}

void Sysinfo::loadProcStat()
{
    m_cpuLoad = 0.0;
    static const QString path = "/proc/stat";

    qDebug() << "Loading CPU load information";

    QVector<unsigned> totalCpuCycles(2), workCpuCycles(2);
    for (int cpuCycleIdx = 0; cpuCycleIdx < totalCpuCycles.length(); ++cpuCycleIdx)
    {
        QFile procStatFile(path); // Force file refresh by reopening it every cycle.
        if (!procStatFile.open(QIODevice::ReadOnly))
        {
            API::Exception("Did not find procstat path: " + path).raise();
        }

        QTextStream parser(&procStatFile);
        getCpuCicles(parser, totalCpuCycles[cpuCycleIdx], workCpuCycles[cpuCycleIdx]);
        QThread::msleep(100); // Sleep here so there is time to update /proc/stat
    }

    const size_t totalCyclesDelta = totalCpuCycles[1] - totalCpuCycles[0];
    const size_t workCyclesDelta = workCpuCycles[1] - workCpuCycles[0];
    m_cpuLoad = (double)workCyclesDelta / totalCyclesDelta * 100.0;
    qInfo() << "Got CPU load";
}

void Sysinfo::loadProcesses()
{
    m_numProcs = 0;
    qDebug() << "Getting loaded processes";

    QDir proc("/proc/");
    if (!proc.exists())
    {
        API::Exception("Did not find proc directory").raise();
    }
    // Processes are in folders identified by their PID alone. Only folder that are number-only matter to us.
    QStringList proc_folders = proc.entryList(QDir::AllDirs);
    for (auto& dir : proc_folders)
    {
        bool digitsOnly = false;
        dir.toInt(&digitsOnly);
        if (digitsOnly)
        {
            ++m_numProcs;
        }
    }
    qInfo() << "Got" << m_numProcs << "running processes";
}

void Sysinfo::getCpuCicles(QTextStream &cpuInfoStream, unsigned &totalCycles, unsigned &workCycles)
{
    totalCycles = workCycles = 0;
    qDebug() << "Loading CPU cycles";

    // Fields in /proc/stat:
    // cpu <user time> <nice time> <system time> <idle time> <iowait time> <irq time> <soft irq time> [<shadow time>]
    // 'cpu' indicates the average load on all cores. Only the first three items show values of busy-time.
    QString fields[8];
    while (fields[0] != "cpu")
    {
        cpuInfoStream >> fields[0];
    }
    cpuInfoStream >> fields[1] >> fields[2] >> fields[3] >> fields[4] >> fields[5] >> fields[6];

    workCycles = fields[1].toInt() + fields[2].toInt() + fields[3].toInt();
    totalCycles = workCycles + fields[4].toInt() + fields[5].toInt() + fields[6].toInt();
    qDebug() << "Got" << totalCycles << "total cycles and " << workCycles << "word cycles";
}

int Sysinfo::toKb(const QString &value, const QString &unit)
{
    // Usually the unit is always 'kB'.
    if (unit.compare("kb", Qt::CaseInsensitive) == 0)
    {
        return value.toInt();
    }
    if (unit.compare("mb", Qt::CaseInsensitive) == 0)
    {
        return value.toInt() * 1024;
    }
    if (unit.compare("gb", Qt::CaseInsensitive) == 0)
    {
        return value.toInt() * 1024 * 1024;
    }
    API::Exception("Bad formatting in unit conversion = " + unit).raise();
    return 0; // Unreachable, but compiler requires.
}

void Sysinfo::startScan()
{
    qDebug() << "Scanning system info";
    loadMeminfo();
    loadProcesses();
    loadProcStat();
    qInfo() << "System scan successful";
    m_packet = API::Packet(0, m_memLoad, m_cpuLoad, m_numProcs);
    emit scanFinished();
}
} // namespace Client
