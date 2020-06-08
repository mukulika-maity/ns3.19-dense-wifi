/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2010 Georgia Institute of Technology
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: George F. Riley <riley@ece.gatech.edu>
 */

#include "ns3/log.h"
#include "ns3/address.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/tcp-socket-factory.h"
#include "ns3/flow-id-tag.h"
#include "ns3/random-variable.h"
#include "chatty-client.h"
#include "mytag.h"

NS_LOG_COMPONENT_DEFINE ("ChattyClient");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (ChattyClient)
		  ;

TypeId
ChattyClient::GetTypeId (void)
{
	static TypeId tid = TypeId ("ns3::ChattyClient")
    		.SetParent<Application> ()
    		.AddConstructor<ChattyClient> ()
    		.AddAttribute ("SendSize", "The amount of data to send each time.",
    				UintegerValue (400),
    				MakeUintegerAccessor (&ChattyClient::m_sendSize),
    				MakeUintegerChecker<uint32_t> (1))
    				.AddAttribute ("Remote", "The address of the destination",
    						AddressValue (),
    						MakeAddressAccessor (&ChattyClient::m_peer),
    						MakeAddressChecker ())
    				.AddAttribute ("MaxTurns",
    						"The total number of turns to send. "
    						"Once these number of turns are sent, "
    						"no data  is sent again. The value zero means "
    						"that there is no limit.",
    						UintegerValue (0),
    						MakeUintegerAccessor (&ChattyClient::m_maxTurns),
    						MakeUintegerChecker<uint32_t> ())
    				.AddAttribute ("Interval",
    							"The time to wait between packets",
    							TimeValue (Seconds (0.1)),
    							MakeTimeAccessor (&ChattyClient::m_interval),
    							MakeTimeChecker ())
    				.AddAttribute ("Protocol", "The type of protocol to use.",
    								TypeIdValue (TcpSocketFactory::GetTypeId ()),
    								MakeTypeIdAccessor (&ChattyClient::m_tid),
    								MakeTypeIdChecker ())
    								.AddTraceSource ("Tx", "A new packet is created and is sent",
    								MakeTraceSourceAccessor (&ChattyClient::m_txTrace));
	return tid;
}


ChattyClient::ChattyClient ()
: m_socket (0),
  m_connected (false),
  m_totTurns (0),
  m_totalRxCount(0),
  m_midRecvCnt(0),
  m_2ndRecvCnt(0),
  m_closeCnt(0)
{
	NS_LOG_FUNCTION (this);
}

ChattyClient::~ChattyClient ()
{
	NS_LOG_FUNCTION (this);
}

void
ChattyClient::SetMaxTurns (uint32_t maxTurns)
{
	NS_LOG_FUNCTION (this << maxTurns);
	m_maxTurns = maxTurns;
}

Ptr<Socket>
ChattyClient::GetSocket (void) const
{
	NS_LOG_FUNCTION (this);
	return m_socket;
}

void
ChattyClient::DoDispose (void)
{
	NS_LOG_FUNCTION (this);

	m_socket = 0;
	// chain up
	Application::DoDispose ();
}

// Application Methods
void ChattyClient::StartApplication (void) // Called at time specified by Start
{
	NS_LOG_FUNCTION (this);

	// Create the socket if not already
	for(int j=0;j<2;j++){
		openConn(j);
	}

}
// Application Methods
void ChattyClient::openConn (int i) // Called at time specified by Start
{
	NS_LOG_FUNCTION (this);

	// Create the socket if not already
	if (!new_socket[i])
	{
		new_socket[i] = Socket::CreateSocket (GetNode (), m_tid);

		// Fatal error if socket type is not NS3_SOCK_STREAM or NS3_SOCK_SEQPACKET
		if (new_socket[i]->GetSocketType () != Socket::NS3_SOCK_STREAM &&
				new_socket[i]->GetSocketType () != Socket::NS3_SOCK_SEQPACKET)
		{
			NS_FATAL_ERROR ("Using ChattySend with an incompatible socket type. "
					"ChattySend requires SOCK_STREAM or SOCK_SEQPACKET. "
					"In other words, use TCP instead of UDP.");
		}

		if (Inet6SocketAddress::IsMatchingType (m_peer))
		{
			new_socket[i]->Bind6 ();
		}
		else if (InetSocketAddress::IsMatchingType (m_peer))
		{
			new_socket[i]->Bind ();
		}
		if(i==0){
			client_sendSize[new_socket[i]]=300;//main page with auth
		}
		else if(i==1){
			client_sendSize[new_socket[i]]=320;//back.gif
		}
		else if(i==2){
					client_sendSize[new_socket[i]]=300;//compressed.gif
		}
		else if(i==3){
					client_sendSize[new_socket[i]]=320;//movie.gif
		}
		else if(i==4){
					client_sendSize[new_socket[i]]=328;//text.gif
		}
		else if(i==5){
					client_sendSize[new_socket[i]]=335;//unknown.gif
		}
		new_socket[i]->Connect (m_peer);
		//m_socket->ShutdownRecv ();
		new_socket[i]->SetConnectCallback (
				MakeCallback (&ChattyClient::ConnectionSucceeded, this),
				MakeCallback (&ChattyClient::ConnectionFailed, this));
		//new_socket[i]->SetSendCallback (
			//	MakeCallback (&ChattyClient::DataSend, this));

		new_socket[i]->SetRecvCallback (
					MakeCallback (&ChattyClient::DataRecv, this));

	}

}

void ChattyClient::StopApplication (void) // Called at time specified by Stop
{
	NS_LOG_FUNCTION (this);

	if (m_socket != 0)
	{
		m_socket->Close ();
		m_connected = false;
	}
	else
	{
		NS_LOG_WARN ("ChattyClient found null socket to close in StopApplication");
	}
}

void ChattyClient::SetFill (std::string fill)
{
	uint32_t dataSize = fill.size () + 1;


	m_dataSize = dataSize;
	memcpy (m_data, fill.c_str (), dataSize);


}
// Private helpers

void ChattyClient::SendData (Ptr<Socket> socket)
{
	NS_LOG_FUNCTION (this);

	// while (m_maxTurns >= m_totTurns)
	//{ // Time to send more
	uint32_t toSend;

	toSend = client_sendSize[socket];

	// Make sure we don't send too many

	NS_LOG_INFO("chatty client sending packet at " << Simulator::Now ().GetSeconds());
	//for(int j=0;j<10;j++){
	Ptr<Packet> packet = Create<Packet> (toSend);

	m_txTrace (packet);

	int actual = socket->Send (packet);
	// We exit this loop when actual < toSend as the send side
	// buffer is full. The "DataSent" callback will pop when
	// some buffer space has freed ip.
	if ((unsigned)actual < toSend)
	{
		NS_LOG_ERROR("Message not sent");
		//break;
	}
	//client_sendCnt[socket]+=1;
	//if(client_sendCnt[socket]==10){
		//	socket->Close();
		//}
	//}


}

void ChattyClient::ConnectionSucceeded (Ptr<Socket> socket)
{
	NS_LOG_FUNCTION (this << socket);
	NS_LOG_LOGIC ("ChattyClient Connection succeeded");
	m_connected = true;

	SendData (socket);


}

void ChattyClient::ConnectionFailed (Ptr<Socket> socket)
{
	NS_LOG_FUNCTION (this << socket);
	NS_LOG_LOGIC ("ChattyClient, Connection Failed");
}
void ChattyClient::close_socket(Ptr<Socket> socket){
	socket->Close ();
	m_connected = false;
}
void ChattyClient::DataRecv(Ptr<Socket> socket)
{
	NS_LOG_FUNCTION (this << socket);
	Ptr<Packet> packet;
	Address from;
	while ((packet = socket->RecvFrom (from)))
	{
		if (packet->GetSize () == 0)
		{ //EOF
			break;
		}
		client_recvCnt[socket]+=1;
		if (InetSocketAddress::IsMatchingType (from))
		{
			NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds ()
					<< "s chatty client received "
					<<  packet->GetSize () << " bytes from "
					<< InetSocketAddress::ConvertFrom(from).GetIpv4 ()
					<< " port " << InetSocketAddress::ConvertFrom (from).GetPort ()
					<< " total Rx " << m_totalRx << " bytes");
		}
		else if (Inet6SocketAddress::IsMatchingType (from))
		{
			NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds ()
					<< "s chatty client received "
					<<  packet->GetSize () << " bytes from "
					<< Inet6SocketAddress::ConvertFrom(from).GetIpv6 ()
					<< " port " << Inet6SocketAddress::ConvertFrom (from).GetPort ()
					<< " total Rx " << m_totalRx << " bytes");
		}
		m_rxTrace (packet, from);

	}

	NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds ()
			<< "s chatty client" <<socket->GetNode()->GetId()<<"receive count "
			<<  m_totalRxCount);
	if(client_recvCnt[socket]<20)
		SendData (socket);
	else if(client_recvCnt[socket]==20){
		client_recvCnt[socket]=0;
	}

	//	socket->Close();


}


} // Namespace ns3
