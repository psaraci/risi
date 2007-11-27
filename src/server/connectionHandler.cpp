/***************************************************************************
 *   Copyright (C) 2007 by Petref Saraci                                   *
 *   psaraci@gmail.com                                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "server/connectionHandler.h"

/**
 * Default public constructor
 * initializes protocol format and protocol version
 * @param socket
 * @param parent
 */
ConnectionHandler::ConnectionHandler( QTcpSocket *socket , QObject *parent )
    :QObject( parent ), client(socket), clientError()
{
    setupConnections();
}

ConnectionHandler::~ConnectionHandler()
{
//     delete client;
}

/**
 * this private functions helps in creating signal-slots connections
 */
void ConnectionHandler::setupConnections()
{
    connect( client, SIGNAL( readyRead() ), this, SLOT( dataArrived() ) );

    connect( NetworkProtocol::instance(), SIGNAL(messageReady(const QByteArray, const qint8, const qint8 )), this, SIGNAL(messageArrived(const QByteArray, const qint8, const qint8 )));

    connect( NetworkProtocol::instance(), SIGNAL(networkProtocolError(NetworkProtocol::ProtocolError) ), this, SLOT(networkProtocolErrorSlot(NetworkProtocol::ProtocolError)) );

//TODO QAbstractSocket::SocketError is not a registered metatype, so for queued connections, you will have to register it with Q_REGISTER_METATYPE
    connect( client, SIGNAL(error( QAbstractSocket::SocketError )), this,SLOT(socketErrors( QAbstractSocket::SocketError )) ) ;

    connect( client, SIGNAL(stateChanged( QAbstractSocket::SocketState ) ), this, SLOT( socketStateChanged(QAbstractSocket::SocketState) ) );

    connect( client, SIGNAL(disconnected()), this, SIGNAL(disconnectMe()) );
}

void ConnectionHandler::sendMessage(QString msg, qint8 type, qint8 gameID )
{
    QByteArray packet = NetworkProtocol::instance()->createPacket( msg, type, gameID );
    qint32 size = NetworkProtocol::instance()->sizeOfPacket( packet );

    //sending the packet size first
    client->write(reinterpret_cast<char*>(&size), sizeof(qint32));
    //sendind the packet itself
    client->write(packet);
}

void ConnectionHandler::dataArrived()
{
    NetworkProtocol::instance()->readData( client );
}

void ConnectionHandler::networkProtocolErrorSlot( NetworkProtocol::ProtocolError err )
{
    switch( err )
    {
        case NetworkProtocol::InvalidFormat:
        {
            qDebug()<<"invalid protocol format";
            sendMessage("", 'a', -1 );
            client->flush();//causes the socket to send the data "immediately"
            client->close();
            clientError = tr("invalid protocol format");//storing the error
            emit disconnectMe();
            break;
        }
        case NetworkProtocol::InvalidVersion:
        {
            qDebug()<<"invalid protocol version";
            sendMessage("", 'b', -1 );
            client->flush();
            client->close();
            clientError = tr("invalid protocol version");
            emit disconnectMe();
            break;
        }
        default:
        {
            qDebug()<<"unkown protocol error";
            sendMessage("", 'c', -1 );
            client->flush();
            client->close();
            clientError = tr("unkown protocol error");
            emit disconnectMe();
        }
    }
}

void ConnectionHandler::socketErrors( QAbstractSocket::SocketError errors)
{
    switch( errors )
    {
        case QAbstractSocket::ConnectionRefusedError:
        {
            qDebug()<<"QAbstractSocket::ConnectionRefusedError ";
            clientError = tr( "The connection was refused by the peer (or timed out) !" ) ;
            break;
        }
        case QAbstractSocket::RemoteHostClosedError:
        {
            qDebug()<<"QAbstractSocket::RemoteHostClosedError";
            clientError = tr( "connection was closed by remote host/client!" );
            break;
        }
        case QAbstractSocket::HostNotFoundError:
        {
            qDebug()<<"QAbstractSocket::HostNotFoundError";
            clientError = tr( " The host address was not found !" );
            break;
        }
        case QAbstractSocket::SocketAccessError:
        {
            qDebug()<<"QAbstractSocket::SocketAccessError";
            clientError = tr( "The socket operation failed because the application lacked the required privileges for accesing sockets !" );
            break;
        }
        case QAbstractSocket::SocketResourceError:
        {
            qDebug()<<"QAbstractSocket::SocketResourceError";
            clientError = tr( "The local system ran out of resources (e.g., too many sockets) !" );
            break;
        }
        case QAbstractSocket::SocketTimeoutError:
        {
            qDebug()<<"QAbstractSocket::SocketTimeoutError";
            clientError = tr( "The socket operation timed out !" );
            break;
        }
        case QAbstractSocket::DatagramTooLargeError:
        {
            qDebug()<<"QAbstractSocket::DatagramTooLargeError";
            clientError = tr( "The datagram was larger than the operating system's limit (which can be as low as 8192 bytes) !" );
            break;
        }
        case QAbstractSocket::NetworkError:
        {
            qDebug()<<"QAbstractSocket::NetworkError";
            clientError = tr( "An error occurred with the network (e.g., the network cable was accidentally plugged out?)" );
            break;
        }
        case QAbstractSocket::AddressInUseError:
        {
            qDebug()<<"QAbstractSocket::AddressInUseError";
            clientError = tr("The address specified to QUdpSocket::bind() is already in use and was set to be exclusive !");
            break;
        }
        case QAbstractSocket::SocketAddressNotAvailableError:
        {
            qDebug()<<"QAbstractSocket::SocketAddressNotAvailableError";
            clientError = tr( "The address specified to QUdpSocket::bind() does not belong to the host ! " );
            break;
        }
        case QAbstractSocket::UnsupportedSocketOperationError:
        {
            qDebug()<<"QAbstractSocket::UnsupportedSocketOperationError";
            clientError = tr( "The requested socket operation is not supported by the local operating system (e.g., lack of IPv6 support) ! " );
            break;
        }
        case QAbstractSocket::ProxyAuthenticationRequiredError:
        {
            qDebug()<<"QAbstractSocket::ProxyAuthenticationRequiredError";
            clientError = tr( "The socket is using a proxy, and the proxy requires authentication" );
            break;
        }
        case QAbstractSocket::UnknownSocketError:
        {
            qDebug()<<"QAbstractSocket::UnknownSocketError";
            clientError = tr("An unidentified error occurred !");
            break;
        }
        case QAbstractSocket::UnfinishedSocketOperationError:
        {
            qDebug()<<"QAbstractSocket::UnfinishedSocketOperationError";
            clientError = tr( "Used by QAbstractSocketEngine only, The last operation attempted has not finished yet (still in progress in the background) !" );
            break;
        }
        default:
            qDebug()<<"some sort of error";
    }

}

void ConnectionHandler::socketStateChanged( QAbstractSocket::SocketState state )
{
    switch( state )
    {
        case QAbstractSocket::UnconnectedState:
        {
            qDebug()<<"QAbstractSocket::UnconnectedState";
            break;
        }
        case QAbstractSocket::HostLookupState:
        {
            qDebug()<<"QAbstractSocket::HostLookupState";
            break;
        }
        case QAbstractSocket::ConnectingState:
        {
            qDebug()<<"QAbstractSocket::ConnectingState";
            break;
        }
        case QAbstractSocket::ConnectedState:
        {
            qDebug()<<"QAbstractSocket::ConnectedState";
            break;
        }
        case QAbstractSocket::BoundState:
        {
            qDebug()<<"QAbstractSocket::BoundState";
            break;
        }
        case QAbstractSocket::ClosingState:
        {
            qDebug()<<"QAbstractSocket::ClosingState";
            break;
        }
        case QAbstractSocket::ListeningState:
        {
            qDebug()<<"QAbstractSocket::ListeningState";
            break;
        }
        default:
            qDebug()<<"some sort of socket state";
    }

}


