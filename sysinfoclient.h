#ifndef SYSINFOCLIENT_H
#define SYSINFOCLIENT_H

#include <QObject>
#include <QWebSocket>
#include "sysinfo.h"

// Client that collects the system information and sends them to the server.
class SysinfoClient : public QObject
{
    Q_OBJECT
public:
    explicit SysinfoClient(const QUrl &url, int clientId, QObject *parent = nullptr);

signals:
    void closed();

public slots:
    void onConnected();
    void onDisconnected();
    void onScanFinished();
    void rescanInformation();

private:
    const int m_clientId;
    const QUrl m_url;
    QWebSocket m_webSocket;
    Client::Sysinfo m_sysinfo;
};

#endif // SYSINFOCLIENT_H
