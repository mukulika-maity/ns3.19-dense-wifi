/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005, 2009 INRIA
 * Copyright (c) 2009 MIRKO BANCHI
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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 * Author: Mirko Banchi <mk.banchi@gmail.com>
 */

#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/core-module.h"

#include "wifi-mac-queue.h"
#include "qos-blocked-destinations.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (WifiMacQueue)
  ;

WifiMacQueue::Item::Item (Ptr<const Packet> packet,
                          const WifiMacHeader &hdr,
                          Time tstamp)
  : packet (packet),
    hdr (hdr),
    tstamp (tstamp)
{
}

TypeId
WifiMacQueue::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::WifiMacQueue")
    .SetParent<Object> ()
    .AddConstructor<WifiMacQueue> ()
    .AddAttribute ("MaxPacketNumber", "If a packet arrives when there are already this number of packets, it is dropped.",
                   UintegerValue (400),
                   MakeUintegerAccessor (&WifiMacQueue::m_maxSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("MaxDelay", "If a packet stays longer than this delay in the queue, it is dropped.",
                   TimeValue (Seconds (10.0)),
                   MakeTimeAccessor (&WifiMacQueue::m_maxDelay),
                   MakeTimeChecker ())
  ;

  return tid;
}

WifiMacQueue::WifiMacQueue ()
  : m_size (0)
{
	//m_uniformRandomVariable = CreateObject<UniformRandomVariable> ();
}

WifiMacQueue::~WifiMacQueue ()
{
  Flush ();
}

void
WifiMacQueue::SetMaxSize (uint32_t maxSize)
{
  m_maxSize = maxSize;
}

void
WifiMacQueue::SetMaxDelay (Time delay)
{
  m_maxDelay = delay;
}

uint32_t
WifiMacQueue::GetMaxSize (void) const
{
  return m_maxSize;
}

Time
WifiMacQueue::GetMaxDelay (void) const
{
  return m_maxDelay;
}

void
WifiMacQueue::Enqueue (Ptr<const Packet> packet, const WifiMacHeader &hdr)
{
  Cleanup ();
  if (m_size == m_maxSize)
    {
	  std::cout<<"buffer overflow at "<<this<<" at time= "<<Simulator::Now().GetSeconds()<<std::endl;
      return;
    }
  std::cout<<"T="<<Simulator::Now().GetSeconds()<<"size="<<m_size<<"enqueue p="<<*packet<<std::endl;

  Time now = Simulator::Now ();
  m_queue.push_back (Item (packet, hdr, now));
  m_size++;
}

void
WifiMacQueue::Cleanup (void)
{
  if (m_queue.empty ())
    {
      return;
    }

  Time now = Simulator::Now ();
  uint32_t n = 0;
  for (PacketQueueI i = m_queue.begin (); i != m_queue.end ();)
    {
      if (i->tstamp + m_maxDelay > now)
        {
          i++;
        }
      else
        {
    	  std::cout<<"T="<<Simulator::Now().GetSeconds()<<"drop p="<<*(i->packet)<<std::endl;
          i = m_queue.erase (i);
          n++;
        }
    }
  m_size -= n;
}

Ptr<const Packet>
WifiMacQueue::Dequeue (WifiMacHeader *hdr)
{
  Cleanup ();
  if (!m_queue.empty ())
    {
      Item i = m_queue.front ();
      m_queue.pop_front ();
      m_size--;
      *hdr = i.hdr;
      std::cout<<"T="<<Simulator::Now().GetSeconds()<<"size="<<m_size<<"dequeue p="<<*i.packet<<std::endl;

      return i.packet;
    }
  return 0;
}
Ptr<const Packet>
WifiMacQueue::DequeueByAddresses (WifiMacHeader *hdr, std::list<Mac48Address> dests,std::list<Mac48Address> m_clients)
{
	//std::cout<<"in begin dequeueByAddresses"<<std::endl;
  Cleanup ();
  Ptr<const Packet> packet = 0;
  if (!m_queue.empty ())
    {
	  Item i = m_queue.front ();

	        //if((i.hdr.GetAddr1()==Mac48Address("ff:ff:ff:ff:ff:ff")) || (i.hdr.IsMgt()) || (i.hdr.IsData() && i.packet->GetSize()==36)){
	        if((i.hdr.GetAddr1()==Mac48Address("ff:ff:ff:ff:ff:ff")) || (i.hdr.IsMgt()) ){
	        	//std::cout<<"at time"<<Simulator::Now().GetSeconds()<<"packet poped of size="<<i.packet->GetSize()<<"of type"<<i.hdr.GetType()<<"to "<<i.hdr.GetAddr1()<<std::endl;
	        	m_queue.pop_front ();
	        	m_size--;
	        	*hdr = i.hdr;

	        	return i.packet;
	        }
	       PacketQueueI it;
	       /* SeedManager::SetSeed (1);
	        		SeedManager::SetRun (1);
	        		UniformVariable var;
	        		int rand = var.GetValue(0,100);
	        		//std::cout<<"var="<<rand<<std::endl;
	        //int coinFlip = m_uniformRandomVariable->GetInteger (0, 100) % 10;

	        if(rand<98){*/

      for (it = m_queue.begin (); it != m_queue.end (); ++it)
        {
    	      std::list<Mac48Address>::iterator clientI;
    	      for (clientI = dests.begin (); clientI != dests.end (); ++clientI){
    	    	  if (*clientI == it->hdr.GetAddr1())
    	    	  {
    	    		 //std::cout<<"match found for addr"<<*clientI<<std::endl;
    	    		  packet = it->packet;
    	    		  *hdr = it->hdr;
    	    		  m_queue.erase (it);
    	    		  m_size--;
    	    		  std::cout<<"T="<<Simulator::Now().GetSeconds()<<"size="<<m_size<<"dequeue p="<<*packet<<std::endl;
    	    		  return packet;
    	    	  }
            }
        }

      //std::cout<<"first for loop end"<<std::endl;
    	    if(it==m_queue.end() && packet==0){
    	    	Item i = m_queue.front ();
    	    	      m_queue.pop_front ();
    	    	      m_size--;
    	    	      *hdr = i.hdr;
    	    	      return i.packet;
    	    }
	       /* }
	        else{
	        	for (it = m_queue.begin (); it != m_queue.end (); ++it)
	        	        {
	        			std::list<Mac48Address>::iterator m_clientI;
	        			for (m_clientI = m_clients.begin (); m_clientI != m_clients.end (); ++m_clientI){
	        				std::list<Mac48Address>::iterator i = std::find(dests.begin(),dests.end(), *m_clientI);
	        				if (i == dests.end() && *m_clientI == it->hdr.GetAddr1() )
	        	    	    	  {
	        	    	    		 //std::cout<<"match found for addr"<<*clientI<<std::endl;
	        	    	    		  packet = it->packet;
	        	    	    		  *hdr = it->hdr;
	        	    	    		  m_queue.erase (it);
	        	    	    		  m_size--;
	        	    	    		  //std::cout<<"match found before break"<<std::endl;
	        	    	    		  return packet;
	        	    	    	  }
	        	            }

	        	        }
	        	if(it==m_queue.end() && packet==0){
	        	    	    	Item i = m_queue.front ();
	        	    	    	      m_queue.pop_front ();
	        	    	    	      m_size--;
	        	    	    	      *hdr = i.hdr;
	        	    	    	      return i.packet;
	        	    	    }

	        }*/

    }
  //std::cout<<"in end dequeueByAddresses"<<std::endl;
  return 0;
}
Ptr<const Packet>
WifiMacQueue::Peek (WifiMacHeader *hdr)
{
  Cleanup ();
  if (!m_queue.empty ())
    {
      Item i = m_queue.front ();
      *hdr = i.hdr;
      return i.packet;
    }
  return 0;
}

Ptr<const Packet>
WifiMacQueue::DequeueByTidAndAddress (WifiMacHeader *hdr, uint8_t tid,
                                      WifiMacHeader::AddressType type, Mac48Address dest)
{
  Cleanup ();
  Ptr<const Packet> packet = 0;
  if (!m_queue.empty ())
    {
      PacketQueueI it;
      for (it = m_queue.begin (); it != m_queue.end (); ++it)
        {
          if (it->hdr.IsQosData ())
            {
              if (GetAddressForPacket (type, it) == dest
                  && it->hdr.GetQosTid () == tid)
                {
                  packet = it->packet;
                  *hdr = it->hdr;
                  m_queue.erase (it);
                  m_size--;
                  break;
                }
            }
        }
    }
  return packet;
}

Ptr<const Packet>
WifiMacQueue::PeekByTidAndAddress (WifiMacHeader *hdr, uint8_t tid,
                                   WifiMacHeader::AddressType type, Mac48Address dest)
{
  Cleanup ();
  if (!m_queue.empty ())
    {
      PacketQueueI it;
      for (it = m_queue.begin (); it != m_queue.end (); ++it)
        {
          if (it->hdr.IsQosData ())
            {
              if (GetAddressForPacket (type, it) == dest
                  && it->hdr.GetQosTid () == tid)
                {
                  *hdr = it->hdr;
                  return it->packet;
                }
            }
        }
    }
  return 0;
}

bool
WifiMacQueue::IsEmpty (void)
{
  Cleanup ();
  return m_queue.empty ();
}

uint32_t
WifiMacQueue::GetSize (void)
{
  return m_size;
}

void
WifiMacQueue::Flush (void)
{
  m_queue.erase (m_queue.begin (), m_queue.end ());
  m_size = 0;
}

Mac48Address
WifiMacQueue::GetAddressForPacket (enum WifiMacHeader::AddressType type, PacketQueueI it)
{
  if (type == WifiMacHeader::ADDR1)
    {
      return it->hdr.GetAddr1 ();
    }
  if (type == WifiMacHeader::ADDR2)
    {
      return it->hdr.GetAddr2 ();
    }
  if (type == WifiMacHeader::ADDR3)
    {
      return it->hdr.GetAddr3 ();
    }
  return 0;
}

bool
WifiMacQueue::Remove (Ptr<const Packet> packet)
{
  PacketQueueI it = m_queue.begin ();
  for (; it != m_queue.end (); it++)
    {
      if (it->packet == packet)
        {
          m_queue.erase (it);
          m_size--;
          return true;
        }
    }
  return false;
}

void
WifiMacQueue::PushFront (Ptr<const Packet> packet, const WifiMacHeader &hdr)
{
  Cleanup ();
  //std::cout<<"in wifi enqueue size="<<WifiMacQueue::m_size<<std::endl;
  if (m_size == m_maxSize)
    {
	  std::cout<<"buffer overflow at "<<this<<" at time= "<<Simulator::Now().GetSeconds()<<std::endl;
      return;
    }
  Time now = Simulator::Now ();
  m_queue.push_front (Item (packet, hdr, now));
  m_size++;
}

uint32_t
WifiMacQueue::GetNPacketsByTidAndAddress (uint8_t tid, WifiMacHeader::AddressType type,
                                          Mac48Address addr)
{
  Cleanup ();
  uint32_t nPackets = 0;
  if (!m_queue.empty ())
    {
      PacketQueueI it;
      for (it = m_queue.begin (); it != m_queue.end (); it++)
        {
          if (GetAddressForPacket (type, it) == addr)
            {
              if (it->hdr.IsQosData () && it->hdr.GetQosTid () == tid)
                {
                  nPackets++;
                }
            }
        }
    }
  return nPackets;
}

Ptr<const Packet>
WifiMacQueue::DequeueFirstAvailable (WifiMacHeader *hdr, Time &timestamp,
                                     const QosBlockedDestinations *blockedPackets)
{
  Cleanup ();
  Ptr<const Packet> packet = 0;
  for (PacketQueueI it = m_queue.begin (); it != m_queue.end (); it++)
    {
      if (!it->hdr.IsQosData ()
          || !blockedPackets->IsBlocked (it->hdr.GetAddr1 (), it->hdr.GetQosTid ()))
        {
          *hdr = it->hdr;
          timestamp = it->tstamp;
          packet = it->packet;
          m_queue.erase (it);
          m_size--;
          return packet;
        }
    }
  return packet;
}

Ptr<const Packet>
WifiMacQueue::PeekFirstAvailable (WifiMacHeader *hdr, Time &timestamp,
                                  const QosBlockedDestinations *blockedPackets)
{
  Cleanup ();
  for (PacketQueueI it = m_queue.begin (); it != m_queue.end (); it++)
    {
      if (!it->hdr.IsQosData ()
          || !blockedPackets->IsBlocked (it->hdr.GetAddr1 (), it->hdr.GetQosTid ()))
        {
          *hdr = it->hdr;
          timestamp = it->tstamp;
          return it->packet;
        }
    }
  return 0;
}

} // namespace ns3
