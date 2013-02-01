//========================================================================
//  This software is free: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License Version 3,
//  as published by the Free Software Foundation.
//
//  This software is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  Version 3 in the file COPYING that came with this distribution.
//  If not, see <http://www.gnu.org/licenses/>.
//========================================================================
/*!
  \file    robocup_ssl_server.cpp
  \brief   C++ Implementation: robocup_ssl_server
  \author  Stefan Zickler, 2009
  \author  Jan Segre, 2012
  \author  Ali Koochakzadeh, 2013
*/
//========================================================================
#include "robocup_ssl_server.h"
#include <QtNetwork>
#include <iostream>
#include "logger.h"

using namespace std;

#ifndef QT_WITHOUT_MULTICAST

RoboCupSSLServer::RoboCupSSLServer(QObject *parent, const quint16 &port, const string &net_address, const string &net_interface) :
    _socket(new QUdpSocket(parent)),
    _port(port),
    _net_address(new QHostAddress(QString(net_address.c_str()))),
    _net_interface(new QNetworkInterface(QNetworkInterface::interfaceFromName(QString(net_interface.c_str()))))
{
    _socket->setSocketOption(QAbstractSocket::MulticastTtlOption, 1);
}

RoboCupSSLServer::~RoboCupSSLServer()
{
    mutex.lock();
    mutex.unlock();
    delete _socket;
    delete _net_address;
    delete _net_interface;
}

void RoboCupSSLServer::change_port(const quint16 & port)
{
    _port = port;
}

void RoboCupSSLServer::change_address(const string & net_address)
{
    delete _net_address;
    _net_address = new QHostAddress(QString(net_address.c_str()));
}

void RoboCupSSLServer::change_interface(const string & net_interface)
{
    delete _net_interface;
    _net_interface = new QNetworkInterface(QNetworkInterface::interfaceFromName(QString(net_interface.c_str())));
}

bool RoboCupSSLServer::send(const SSL_WrapperPacket & packet)
{
    QByteArray datagram;

    datagram.resize(packet.ByteSize());
    bool success = packet.SerializeToArray(datagram.data(), datagram.size());
    if(!success) {
        //TODO: print useful info
        logStatus(QString("Serializing packet to array failed."), QColor("red"));
        return false;
    }

    mutex.lock();
    quint64 bytes_sent = _socket->writeDatagram(datagram, *_net_address, _port);
    mutex.unlock();
    if (bytes_sent != datagram.size()) {
        logStatus(QString("Sending UDP datagram failed (maybe too large?). Size was: %1 byte(s).").arg(datagram.size()), QColor("red"));
        return false;
    }

    return true;
}

bool RoboCupSSLServer::send(const SSL_DetectionFrame & frame)
{
    SSL_WrapperPacket pkt;
    SSL_DetectionFrame * nframe = pkt.mutable_detection();
    nframe->CopyFrom(frame);
    return send(pkt);
}

bool RoboCupSSLServer::send(const SSL_GeometryData & geometry)
{
    SSL_WrapperPacket pkt;
    SSL_GeometryData * gdata = pkt.mutable_geometry();
    gdata->CopyFrom(geometry);
    return send(pkt);
}

#else 


#include "robocup_ssl_server.h"

RoboCupSSLServer::RoboCupSSLServer(int port,
                     string net_address,
                     string net_interface)
{
  _port=port;
  _net_address=net_address;
  _net_interface=net_interface;

}


RoboCupSSLServer::~RoboCupSSLServer()
{
}

void RoboCupSSLServer::close() {
  mc.close();
}

bool RoboCupSSLServer::open() {
  close();

  if(!mc.open(_port,true,true)) {
    logStatus(QString("Unable to open UDP network port: %1").arg(_port),QColor("red"));
    return(false);
  }

  Net::Address multiaddr,interface;
  multiaddr.setHost(_net_address.c_str(),_port);
  if(_net_interface.length() > 0){
    interface.setHost(_net_interface.c_str(),_port);
  }else{
    interface.setAny();
  }

  if(!mc.addMulticast(multiaddr,interface)) {
    logStatus(QString("Unable to setup UDP multicast."),QColor("orange"));
  }
  logStatus(QString("Vision UDP network successfully configured. (Multicast address: %1:%2)").arg(_net_address.c_str()).arg(_port),QColor("green"));
  return(true);
}

bool RoboCupSSLServer::send(const SSL_WrapperPacket & packet) {
  string buffer;
  packet.SerializeToString(&buffer);
  Net::Address multiaddr;
  multiaddr.setHost(_net_address.c_str(),_port);
  bool result;
  mutex.lock();
  result=mc.send(buffer.c_str(),buffer.length(),multiaddr);
  mutex.unlock();
  if (result==false) {
    logStatus(QString("Sending UDP datagram failed (maybe too large?). Size was: %1 byte(s)").arg(buffer.length()),QColor("red"));
  }
  return(result);
}

bool RoboCupSSLServer::send(const SSL_DetectionFrame & frame) {
  SSL_WrapperPacket pkt;
  SSL_DetectionFrame * nframe = pkt.mutable_detection();
  nframe->CopyFrom(frame);
  return send(pkt);
}

bool RoboCupSSLServer::send(const SSL_GeometryData & geometry) {
  SSL_WrapperPacket pkt;
  SSL_GeometryData * gdata = pkt.mutable_geometry();
  gdata->CopyFrom(geometry);
  return send(pkt);
}

#endif

#ifdef Q_OS_WIN32
#include "robocup_ssl_server.h"


RoboCupSSLServer::RoboCupSSLServer(QObject* parent,int port,
                     string net_address,
                     string net_interface)
{
  _port=port;
  _net_address=net_address;
  _net_interface=net_interface;
  _parent=parent;
  s = NULL;
}


RoboCupSSLServer::~RoboCupSSLServer()
{
}

void RoboCupSSLServer::close() {
    if (s!=NULL)
    {
        s->close();
        delete s;
        s = NULL;
    }
}

bool RoboCupSSLServer::open() {
    close();
    s = new QUdpSocket(_parent);
    ip_mreq Tmreq;
    s->bind(_port,QUdpSocket::ShareAddress);
    int sd = s->socketDescriptor();
    if(sd != -1)
    {
        struct sockaddr_in SendAddr;
        memset(&SendAddr,0,sizeof(SendAddr));
        SendAddr.sin_family=AF_INET;
        SendAddr.sin_addr.s_addr=inet_addr(_net_address.c_str());
        SendAddr.sin_port=htons(_port);
        ip_mreq mreq;
        memset(&mreq,0,sizeof(ip_mreq));
        mreq.imr_multiaddr.s_addr = inet_addr(_net_address.c_str());
        mreq.imr_interface.s_addr = htons(INADDR_ANY);

        //Make this a member of the Multicast Group
        if(setsockopt(sd, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char far *)&mreq,sizeof(mreq)) < 0)
        {
            logStatus(QString("Multicast Init: Membership Error!"),QColor("orange"));
        }
        // set TTL (Time To Live)
        unsigned int ttl = 38;
        if (setsockopt(sd, IPPROTO_IP, IP_MULTICAST_TTL, (const char *)&ttl, sizeof(ttl) ) < 0)
        {
            logStatus(QString("Multicast Init: Time To Live Error!"),QColor("orange"));
        }

    }
    else logStatus(QString("Unable to open UDP network port: %1").arg(_port),QColor("red"));
    //Set up a socket for sending multicast data
    sd = socket(AF_INET, SOCK_DGRAM, 0);
    sinStruct.sin_family = AF_INET;
    sinStruct.sin_port = htons(_port);
    sinStruct.sin_addr.s_addr = inet_addr(_net_address.c_str());
    unsigned int ttl = 38; // 38 hops allowed
    setsockopt(sd, IPPROTO_IP, IP_MULTICAST_TTL, (const char *)&ttl, sizeof(ttl) );

    memset(&Tmreq,0,sizeof(ip_mreq));
    Tmreq.imr_multiaddr.s_addr = inet_addr(_net_address.c_str());
    Tmreq.imr_interface.s_addr = htons(INADDR_ANY);

    setsockopt(sd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
    (char far *)&Tmreq,sizeof(Tmreq));

  return(true);
}

bool RoboCupSSLServer::send(const SSL_WrapperPacket & packet) {
  string buffer;
  packet.SerializeToString(&buffer);
  bool result=true;
  mutex.lock();
  if (s->writeDatagram(buffer.c_str(),buffer.length(),QHostAddress(_net_address.c_str()),_port)==-1)
      result = false;
  //if (sendto(sd, buffer.c_str(), buffer.length(), 0, (struct sockaddr *)&sinStruct, sizeof(sinStruct)) < 0)
//      result = false;
  mutex.unlock();
  if (result==false) {
    logStatus(QString("Sending UDP datagram failed (maybe too large?). Size was: %1 byte(s)").arg(buffer.length()),QColor("red"));
  }
   return(result);
}

bool RoboCupSSLServer::send(const SSL_DetectionFrame & frame) {
  SSL_WrapperPacket pkt;
  SSL_DetectionFrame * nframe = pkt.mutable_detection();
  nframe->CopyFrom(frame);
  return send(pkt);
}

bool RoboCupSSLServer::send(const SSL_GeometryData & geometry) {
  SSL_WrapperPacket pkt;
  SSL_GeometryData * gdata = pkt.mutable_geometry();
  gdata->CopyFrom(geometry);
  return send(pkt);
}

#endif



