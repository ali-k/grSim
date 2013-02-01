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
  \file    robocup_ssl_client.cpp
  \brief   C++ Implementation: robocup_ssl_client
  \author  Stefan Zickler, 2009
  \author  Jan Segre, 2012
  \author  Ali Koochakzadeh, 2013
*/
//========================================================================
#include "robocup_ssl_client.h"
#include <QtNetwork>
#include <iostream>

using namespace std;


#ifndef QT_WITHOUT_MULTICAST

RoboCupSSLClient::RoboCupSSLClient(const quint16 & port, const string & net_address, const string & net_interface) :
    _socket(new QUdpSocket()),
    _port(port),
    _net_address(new QHostAddress(QString(net_address.c_str()))),
    _net_interface(new QNetworkInterface(QNetworkInterface::interfaceFromName(QString(net_interface.c_str()))))
{
}

RoboCupSSLClient::RoboCupSSLClient(const quint16 & port, const QHostAddress & addr, const QNetworkInterface & iface) :
    _socket(new QUdpSocket()),
    _port(port),
    _net_address(new QHostAddress(addr)),
    _net_interface(new QNetworkInterface(iface))
{
}

RoboCupSSLClient::~RoboCupSSLClient()
{
    delete _socket;
    delete _net_address;
    delete _net_interface;
}

void RoboCupSSLClient::close()
{
    if(_socket->state() == QUdpSocket::BoundState)
        _socket->leaveMulticastGroup(*_net_address, *_net_interface);
}

bool RoboCupSSLClient::open()
{
    close();
    if(!_socket->bind(_port, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
        cerr << "Unable to bind UDP socket on port " << _port << ". "
             << _socket->errorString().toStdString() << '.' << endl;
        return false;
    }

    if(!_socket->joinMulticastGroup(*_net_address, *_net_interface)) {
        cerr << "Unable to join UDP multicast on "
             << _net_address->toString().toStdString() << ':' << _port << ". "
             << _socket->errorString().toStdString().c_str() << '.' << endl;
        return false;
    }

    return true;
}

bool RoboCupSSLClient::receive(SSL_WrapperPacket & packet)
{
    if(_socket->state() == QUdpSocket::BoundState && _socket->hasPendingDatagrams()) {
        QByteArray datagram;
        mutex.lock();
        datagram.resize(_socket->pendingDatagramSize());
        _socket->readDatagram(datagram.data(), datagram.size());
        mutex.unlock();
        //decode packet:
        return packet.ParseFromArray(datagram.data(), datagram.size());
    } else {
        return false;
    }
}



#else


RoboCupSSLClient::RoboCupSSLClient(int port,
                     string net_address,
                     string net_interface)
{
  _port=port;
  _net_address=net_address;
  _net_interface=net_interface;
  in_buffer=new char[65536];
}


RoboCupSSLClient::~RoboCupSSLClient()
{
  delete[] in_buffer;
}

void RoboCupSSLClient::close() {
  mc.close();
}

bool RoboCupSSLClient::open(bool blocking) {
  close();
  if(!mc.open(_port,true,true,blocking)) {
    fprintf(stderr,"Unable to open UDP network port: %d\n",_port);
    fflush(stderr);
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
    fprintf(stderr,"Unable to setup UDP multicast\n");
    fflush(stderr);
    return(false);
  }

  return(true);
}

bool RoboCupSSLClient::receive(SSL_WrapperPacket & packet) {
  Net::Address src;
  int r=0;
  r = mc.recv(in_buffer,MaxDataGramSize,src);
  if (r>0) {
    fflush(stdout);
    //decode packet:
    return packet.ParseFromArray(in_buffer,r);
  }
  return false;
}

#endif

