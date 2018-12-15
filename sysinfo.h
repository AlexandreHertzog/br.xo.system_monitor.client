#ifndef CSYSINFO_H
#define CSYSINFO_H

#include <QObject>
#include <QStringList>
#include <QTextStream>
#include "../sysinfo_api/packet.h"

namespace Client
{
// Collects the system information: CPU use, memory use and process count.
// It accomplishes that by reading from the required '/proc/' files, like
// 'stat', 'meminfo' and process-folder hierarchies.
class Sysinfo : public QObject
{
    Q_OBJECT
public:
    Sysinfo();
    API::Packet makePacket(int clientId) const;

public slots:
    void startScan();

signals:
    void scanFinished();

private:
    double m_memLoad;
    double m_cpuLoad;
    int m_numProcs;
    API::Packet m_packet;

    static int toKb(const QString &value, const QString &unit);
    static void getCpuCicles(QTextStream& cpu_file, unsigned &total_cycles, unsigned &work_cycles);

    void loadMeminfo();
    void loadProcStat();
    void loadProcesses();
};
} // namespace Client

#endif // CSysInfo_H
