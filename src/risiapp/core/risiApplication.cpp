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

#include <QFile>
#include <QMessageBox>
#include <QSettings>
#include <QNetworkInterface>

#include "server.h"
#include "risiUI.h"
#include "httpControler.h"
#include "tcpClient.h"
#include "protocol.h"

#include "gameListXML.h"
#include "risiApplication.h"

RISIapplication* RISIapplication::inst = 0;

/**
 * \class RISIapplication
 *
 * \brief Constructor
 * @param parent parent QObject
 */
RISIapplication::RISIapplication( QObject *parent )
    :QObject( parent ), protocol( Protocol::instance() ), http ( HttpControler::instance() ), tcpClients(), serverErrors(), xmlFile( new QFile("gameList.xml") )
{
}

/**
 * \brief returns the instance of RISIapplication
 * @return RISIapplication *
 */
RISIapplication* RISIapplication::instance()
{
    if (inst == 0)
        inst = new RISIapplication;
    return inst;
}

/**
 * \brief starts the server
 */
void RISIapplication::initServer()
{
    server = Server::instance();

    setupConnections();

    QSettings settings;
    settings.beginGroup( "Server" );
    QVariant v = settings.value( "port", 2222 );
    settings.endGroup();

    bool ok;
    int port = v.toInt( &ok );

    if( ok )
    {
        server->listen(QHostAddress::Any, port );
        emit updateServerStatus( server->isListening() );
    }
    else
        QMessageBox::warning(0, tr("Error: "), tr("Internal application error due to conversion from QVariant to integer! Please, try restart the application and submit a bug report ! ") );
}

/**
 * \brief initializes the UI
 */
void RISIapplication::initUI()
{
    risiUI = new RISIui( 0 );
    risiUI->show();
}

/**
 * \brief Destructor
 */
RISIapplication::~RISIapplication()
{
    delete xmlFile;
    delete risiUI;

    server->close();
    delete server;
    delete protocol;

    delete Server::instance();
    delete Protocol::instance();
    delete NetworkProtocol::instance();
}

/**
 * \brief takes a QStandardItemModel and populates it with data read from the xmlFile
 * @param m QStandardItemModel
 */
void RISIapplication::gameListXMLrequest( QStandardItemModel *m ) //FIXME should be QAbstractItemModel
{
    GameListXML gameListXmlParser( m );
    gameListXmlParser.readXML( xmlFile );
}

/**
 * \brief takes a QStandardItemModel and saves it in a xml file
 * @param model QStandardItemModel
 */
void RISIapplication::saveGameListXML( QStandardItemModel *model )//FIXME should be QAbstractItemModel
{
    GameListXML gameListXmlParser( model );
    gameListXmlParser.writeXML( xmlFile );
}

/**
 * \brief sets the SIGNAL-SLOT connections of RISIapplication
 * \internal
 */
void RISIapplication::setupConnections()
{
    connect( risiUI, SIGNAL(connectToIPSignal(const QString, const int)), this, SLOT(connectToServer(const QString, const int)) );

    connect( risiUI, SIGNAL(goOnlineSignal(const QString, const bool)), this, SLOT(goOnlineSlot(const QString, const bool)) );


}

/**
 * \brief when a player is disconnected this function is called, so this client will be removed from the list and notifying the other users
 * @param client TcpClient *
 */
void RISIapplication::playerDisconnected( TcpClient *client )
{
    QString error = client->lastError();

    int instances = tcpClients.removeAll( client );

    if( instances != 0 )
        QMessageBox::warning(0, tr("Warning ! "), tr(" Player disconnected due to: ")+error+" !" );
}

/**
 * \brief
 * @param nickName QString
 * @param onlineStatus bool
 */
void RISIapplication::goOnlineSlot( const QString nickName, const bool onlineStatus ) //TODO docs
{
    bool status;
    if( onlineStatus )
        status = http->goOnline( nickName );
    else
        status = http->goOffline();

    emit updateOnlineStatus( status );

    server->updateOnlineStatus( status );
}

/**
 * \brief
 * @param ip
 * @param port
 */
void RISIapplication::connectToServer( const QString ip, const int port ) //TODO docs ?
{
    if( !isConnectedTo(ip, port) )
    {
        tcpClients.append( new TcpClient( this ) );
        tcpClients.last()->connectToServer( ip, port );
    }
    else
        QMessageBox::warning(0, tr("WARNING ! "), tr("Seems that you are already connected to: ")+ip+" !!" );
}

/**
 * \brief this functions checks whether the user is already connected to the given ip and port
 * @param ip QString
 * @param port int
 * @return bool
 */
bool RISIapplication::isConnectedTo(const QString ip, const int port )
{
    //checks if there are any connections to any server
    if( tcpClients.count() > 0 )
    {
        //looping through the clients
        for( int i = 0; i < tcpClients.count(); ++i )
        {
            TcpClient *client = tcpClients.at( i );
            if( client->serverIP() == ip && client->serverPort() == port )
                return true;
        }
    }

    return false;
}

/**
 * \brief iterates through all the available network interfaces on the system and returns a list of IP address
 * @return QList
 */
QList <QString> RISIapplication::broadcastIPaddresses() const
{
        QList <QHostAddress> hostAddresses = QNetworkInterface::allAddresses();
        QList < QString > addresses;
        QHostAddress networkAddress(QHostAddress::Broadcast);

        for( int i=0; i<hostAddresses.count(); ++i)
        {
            QHostAddress address = hostAddresses.at(i);

            if( address.protocol() ==  networkAddress.protocol() )
               addresses.append( address.toString() );
        }

        return addresses;
}

/**
 * \brief returns the port the server is running on
 * @return qint16
 */
qint16 RISIapplication::serverPort() const
{
    return server->serverPort();
}

/**
 * \brief returns the number of connected players
 * @return int
 */
int RISIapplication::numberOfConnectedPlayers() const
{
    return server->numberOfConnectedPlayers();
}
