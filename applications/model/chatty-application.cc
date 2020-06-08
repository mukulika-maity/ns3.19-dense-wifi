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
#include "chatty-application.h"
#include "mytag.h"

NS_LOG_COMPONENT_DEFINE ("ChattyApplication");

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (ChattyApplication)
		  ;

TypeId
ChattyApplication::GetTypeId (void)
{
	static TypeId tid = TypeId ("ns3::ChattyApplication")
    		.SetParent<Application> ()
    		.AddConstructor<ChattyApplication> ()
    		.AddAttribute ("SendSize", "The amount of data to send each time.",
    				UintegerValue (400),
    				MakeUintegerAccessor (&ChattyApplication::m_sendSize),
    				MakeUintegerChecker<uint32_t> (1))
    				.AddAttribute ("Remote", "The address of the destination",
    						AddressValue (),
    						MakeAddressAccessor (&ChattyApplication::m_peer),
    						MakeAddressChecker ())
    				.AddAttribute ("MaxTurns",
    						"The total number of turns to send. "
    						"Once these number of turns are sent, "
    						"no data  is sent again. The value zero means "
    						"that there is no limit.",
    						UintegerValue (0),
    						MakeUintegerAccessor (&ChattyApplication::m_maxTurns),
    						MakeUintegerChecker<uint32_t> ())
    				.AddAttribute ("Interval",
    							"The time to wait between packets",
    							TimeValue (Seconds (0.1)),
    							MakeTimeAccessor (&ChattyApplication::m_interval),
    							MakeTimeChecker ())
    				.AddAttribute ("Protocol", "The type of protocol to use.",
    								TypeIdValue (TcpSocketFactory::GetTypeId ()),
    								MakeTypeIdAccessor (&ChattyApplication::m_tid),
    								MakeTypeIdChecker ())
    								.AddTraceSource ("Tx", "A new packet is created and is sent",
    								MakeTraceSourceAccessor (&ChattyApplication::m_txTrace));
	return tid;
}


ChattyApplication::ChattyApplication ()
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

ChattyApplication::~ChattyApplication ()
{
	NS_LOG_FUNCTION (this);
}

void
ChattyApplication::SetMaxTurns (uint32_t maxTurns)
{
	NS_LOG_FUNCTION (this << maxTurns);
	m_maxTurns = maxTurns;
}

Ptr<Socket>
ChattyApplication::GetSocket (void) const
{
	NS_LOG_FUNCTION (this);
	return m_socket;
}

void
ChattyApplication::DoDispose (void)
{
	NS_LOG_FUNCTION (this);

	m_socket = 0;
	// chain up
	Application::DoDispose ();
}

// Application Methods
void ChattyApplication::StartApplication (void) // Called at time specified by Start
{
	NS_LOG_FUNCTION (this);

	// Create the socket if not already
	if (!m_socket)
	{
		m_socket = Socket::CreateSocket (GetNode (), m_tid);

		// Fatal error if socket type is not NS3_SOCK_STREAM or NS3_SOCK_SEQPACKET
		if (m_socket->GetSocketType () != Socket::NS3_SOCK_STREAM &&
				m_socket->GetSocketType () != Socket::NS3_SOCK_SEQPACKET)
		{
			NS_FATAL_ERROR ("Using ChattySend with an incompatible socket type. "
					"ChattySend requires SOCK_STREAM or SOCK_SEQPACKET. "
					"In other words, use TCP instead of UDP.");
		}

		if (Inet6SocketAddress::IsMatchingType (m_peer))
		{
			m_socket->Bind6 ();
		}
		else if (InetSocketAddress::IsMatchingType (m_peer))
		{
			m_socket->Bind ();
		}
		client_sendSize[m_socket]=311;//get main page w/0 auth
		m_socket->Connect (m_peer);
		//m_socket->ShutdownRecv ();
		m_socket->SetConnectCallback (
				MakeCallback (&ChattyApplication::ConnectionSucceeded, this),
				MakeCallback (&ChattyApplication::ConnectionFailed, this));
	//	m_socket->SetSendCallback (
		//		MakeCallback (&ChattyApplication::DataSend, this));
		m_socket->SetRecvCallback (
				MakeCallback (&ChattyApplication::DataRecv, this));
	}

}
// Application Methods
void ChattyApplication::openConn (int i) // Called at time specified by Start
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
			client_sendSize[new_socket[i]]=574;//main page with auth
		}
		else if(i==1){
			client_sendSize[new_socket[i]]=320;//back.gif
		}
		else if(i==2){
					client_sendSize[new_socket[i]]=332;//compressed.gif
		}
		else if(i==3){
					client_sendSize[new_socket[i]]=325;//movie.gif
		}
		else if(i==4){
					client_sendSize[new_socket[i]]=328;//text.gif
		}
		else if(i==5){
					client_sendSize[new_socket[i]]=335;//unknown.gif
		}
		else if(i==6){
					client_sendSize[new_socket[i]]=685;
		}
		new_socket[i]->Connect (m_peer);
		//m_socket->ShutdownRecv ();
		new_socket[i]->SetConnectCallback (
				MakeCallback (&ChattyApplication::ConnectionSucceeded, this),
				MakeCallback (&ChattyApplication::ConnectionFailed, this));
		//new_socket[i]->SetSendCallback (
			//	MakeCallback (&ChattyApplication::DataSend, this));
		if(i==0){
			new_socket[i]->SetRecvCallback (
					MakeCallback (&ChattyApplication::DataRecv1, this));
		}
		else if(i==2){
			new_socket[i]->SetRecvCallback (
					MakeCallback (&ChattyApplication::DataRecvMiddleClient, this));
		}
		else{
			new_socket[i]->SetRecvCallback (
					MakeCallback (&ChattyApplication::DataRecvNewClient, this));
		}
	}

}

void ChattyApplication::StopApplication (void) // Called at time specified by Stop
{
	NS_LOG_FUNCTION (this);

	if (m_socket != 0)
	{
		m_socket->Close ();
		m_connected = false;
	}
	else
	{
		NS_LOG_WARN ("ChattyApplication found null socket to close in StopApplication");
	}
}

void ChattyApplication::SetFill (std::string fill)
{
	uint32_t dataSize = fill.size () + 1;


	m_dataSize = dataSize;
	memcpy (m_data, fill.c_str (), dataSize);


}
// Private helpers

void ChattyApplication::SendData (Ptr<Socket> socket)
{
	NS_LOG_FUNCTION (this);

	// while (m_maxTurns >= m_totTurns)
	//{ // Time to send more
	uint32_t toSend = client_sendSize[socket];
	// Make sure we don't send too many

	NS_LOG_INFO("chatty client sending packet at " << Simulator::Now ().GetSeconds());

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


}
/*void ChattyApplication::ScheduleTransmit (Time dt)
{
	NS_LOG_FUNCTION (this << dt);
	m_sendEvent = Simulator::Schedule (dt, &ChattyApplication::SendData, this);
}*/

void ChattyApplication::ConnectionSucceeded (Ptr<Socket> socket)
{
	NS_LOG_FUNCTION (this << socket);
	NS_LOG_LOGIC ("ChattyApplication Connection succeeded");
	m_connected = true;
	SendData (socket);

}

void ChattyApplication::ConnectionFailed (Ptr<Socket> socket)
{
	NS_LOG_FUNCTION (this << socket);
	NS_LOG_LOGIC ("ChattyApplication, Connection Failed");
}

/*void ChattyApplication::DataSend (Ptr<Socket> s, uint32_t)
{
	NS_LOG_FUNCTION (this);

	if (m_connected)
	{ // Only send new data if the connection has completed
		Simulator::ScheduleNow (&ChattyApplication::SendData, s, this);
	}
}*/
void ChattyApplication::DataRecv (Ptr<Socket> socket)
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
		m_totalRx += packet->GetSize ();
		m_totalRxCount+=1;
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
	close_socket(socket);
	// int seed = (i+1)*(j+1);
	//SeedManager::SetSeed (seed);
	//SeedManager::SetRun (1);
	UniformVariable var;
	int rand = var.GetInteger(10,25);

	Time dt=Seconds(Simulator::Now().GetSeconds());
	//Time dt=Seconds(rand);
	std::cout<<"time now="<<Simulator::Now().GetSeconds()<<"rand="<<rand<<"var in chatty="<<dt.GetSeconds()<<std::endl;
	m_sendEvent1 = Simulator::Schedule (dt, &ChattyApplication::openConn, this,0);
	//openConn(0);
}

void ChattyApplication::close_socket(Ptr<Socket> socket){
	socket->Close ();
	m_closeCnt+=1;
	if(m_closeCnt==7){
		//m_sendSize=300;
		UniformVariable var;
		int rand = var.GetInteger(1,5);

		//Time dt=Seconds(Simulator::Now().GetSeconds())+Seconds(rand);
		Time dt=Seconds(rand);
		std::cout<<"time now="<<Simulator::Now().GetSeconds()<<"rand="<<rand<<"var in close="<<dt.GetSeconds()<<std::endl;
		m_sendEvent2 = Simulator::Schedule (dt, &ChattyApplication::openConn, this,6);
		//openConn(6);

	}
	m_connected = false;
}
void ChattyApplication::DataRecv1 (Ptr<Socket> socket)
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
		m_totalRx += packet->GetSize ();
		m_totalRxCount+=1;
		m_2ndRecvCnt+=1;
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
	if(m_2ndRecvCnt==1){
	client_sendSize[new_socket[0]]=327;//blank.gif
	SendData(socket);
	openConn(1);
	openConn(2);
	openConn(3);
	openConn(4);
	openConn(5);
	}
}
void ChattyApplication::DataRecvNewClient (Ptr<Socket> socket)
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
		m_totalRx += packet->GetSize ();
		m_totalRxCount+=1;
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

}
void ChattyApplication::DataRecvMiddleClient (Ptr<Socket> socket)
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
		m_midRecvCnt+=1;
		m_totalRx += packet->GetSize ();
		m_totalRxCount+=1;
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
	if(m_midRecvCnt==1){
		client_sendSize[new_socket[2]]=269;//favicon.ico
		SendData(socket);
	}
	else if(m_midRecvCnt==2){
			client_sendSize[new_socket[2]]=299;//favicon.ico 2nd time
			SendData(socket);
		}
	else if(m_midRecvCnt==3){
		close_socket(new_socket[0]);
		close_socket(new_socket[1]);
		close_socket(new_socket[2]);
		close_socket(new_socket[3]);
		close_socket(new_socket[4]);
		close_socket(new_socket[5]);
	}

}


} // Namespace ns3
