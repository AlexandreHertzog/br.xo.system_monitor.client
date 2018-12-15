#include "sysinfoclient.h"
#include <QDebug>
#include <QJsonDocument>

SysinfoClient::SysinfoClient(const QUrl &url, int clientId, QObject *parent) : QObject(parent),
    m_clientId(clientId),
    m_url(url)
{
    qDebug() << "WebSocket server:" << url;
    connect(&m_sysinfo, &Client::Sysinfo::scanFinished, this, &SysinfoClient::onScanFinished);
    connect(&m_webSocket, &QWebSocket::connected, this, &SysinfoClient::onConnected);
    connect(&m_webSocket, &QWebSocket::disconnected, this, &SysinfoClient::onDisconnected);
}

void SysinfoClient::rescanInformation()
{
    m_sysinfo.startScan();
}

void SysinfoClient::onScanFinished()
{
    qDebug() << "Scan complete, connecting to" << m_url;
    m_webSocket.open(QUrl(m_url));
}

void SysinfoClient::onConnected()
{
    const QByteArray json = QJsonDocument(m_sysinfo.makePacket(m_clientId).toJson()).toJson();
    qDebug() << "WebSocket connected, sending" << json;
    m_webSocket.sendTextMessage(json);
    m_webSocket.close();
}

void SysinfoClient::onDisconnected()
{
    qDebug() << "Websocket disconnected";
}
