/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 */

#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"

// Default Network Topology
//
//   Wifi 10.1.3.0
//                 AP
//  *    *    *    *
//  |    |    |    |    10.1.1.0
// n5   n6   n7   n0 -------------- n1

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("MyWireless");

int
main (int argc, char *argv[])
{
	bool verbose = true;
	uint32_t nCsma = 2;
	uint32_t nWifi = 20;
	uint32_t maxBytes = 9437184;
	//uint32_t chattyBytes = 102400;
	bool tracing = true;
	Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (1448));
	CommandLine cmd;
	cmd.AddValue ("nCsma", "Number of \"extra\" CSMA nodes/devices", nCsma);
	cmd.AddValue ("nWifi", "Number of wifi STA devices", nWifi);
	cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);

	cmd.Parse (argc,argv);
	//SeedManager::SetSeed (99);
	//SeedManager::SetRun (1);
	if (nWifi > 50)
	{
		std::cout << "Number of wifi nodes " << nWifi <<
				" specified exceeds the mobility bounding box" << std::endl;
		exit (1);
	}

	if (verbose)
	{
		LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
		LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
	}


	NodeContainer csmaNodes;
	csmaNodes.Create (2);

	CsmaHelper csma;
	csma.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
	csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));

	NetDeviceContainer csmaDevices;
	csmaDevices = csma.Install (csmaNodes);

	NodeContainer wifiStaNodes;
	wifiStaNodes.Create (nWifi);
	NodeContainer wifiApNode = csmaNodes.Get (0);

	YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
	YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
	phy.SetChannel (channel.Create ());

	WifiHelper wifi;
	wifi.SetStandard (WIFI_PHY_STANDARD_80211g);
	//wifi.SetRemoteStationManager ("ns3::IdealWifiManager");
	wifi.SetRemoteStationManager ("ns3::AarfWifiManager");
	NqosWifiMacHelper mac = NqosWifiMacHelper::Default ();

	Ssid ssid = Ssid ("ns-3-ssid");
	mac.SetType ("ns3::StaWifiMac",
			"Ssid", SsidValue (ssid),
			"ActiveProbing", BooleanValue (false));

	NetDeviceContainer staDevices;
	staDevices = wifi.Install (phy, mac, wifiStaNodes);

	mac.SetType ("ns3::ApWifiMac",
			"Ssid", SsidValue (ssid));

	NetDeviceContainer apDevices;
	apDevices = wifi.Install (phy, mac, wifiApNode);
	/* Ptr<WifiNetDevice> dev = DynamicCast<WifiNetDevice> (apDevices.Get (0));
       NS_ASSERT (dev != NULL);
       PointerValue ptr;
       dev->GetAttribute ("Mac",ptr);
       Ptr<RegularWifiMac> regmac = ptr.Get<RegularWifiMac> ();
       NS_ASSERT (regmac != NULL);
       regmac->GetAttribute ("DcaTxop",ptr);
       Ptr<DcaTxop> dca = ptr.Get<DcaTxop> ();
       NS_ASSERT (dca != NULL);
       Ptr<WifiMacQueue> queue = dca->GetQueue ();
       NS_ASSERT (queue != NULL);
       std::cout<<"Queue size: " << queue->GetMaxSize()<<std::endl;
       queue->SetMaxSize(10000);
       std::cout<<"Queue size: " << queue->GetMaxSize()<<std::endl;*/
	MobilityHelper mobility;



	mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	mobility.Install (wifiStaNodes);

	mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	mobility.Install (wifiApNode);

	InternetStackHelper stack;
	stack.Install (csmaNodes);
	//stack.Install (wifiApNode);
	stack.Install (wifiStaNodes);

	Ipv4AddressHelper address;


	address.SetBase ("10.1.2.0", "255.255.255.0");
	Ipv4InterfaceContainer csmaInterfaces;
	csmaInterfaces = address.Assign (csmaDevices);

	address.SetBase ("10.1.3.0", "255.255.255.0");
	Ipv4InterfaceContainer wifiInterfaces;
	wifiInterfaces=address.Assign (staDevices);
	address.Assign (apDevices);

	uint16_t Chattyport = 9;  // well-known echo port number
		UdpEchoServerHelper server (Chattyport);
		ApplicationContainer UdpServerapps = server.Install (csmaNodes.Get (nCsma-1));
		UdpServerapps.Start (Seconds (0.0));
		UdpServerapps.Stop (Seconds (10.0));

		//
		// Create a UdpEchoClient application to send UDP datagrams from node zero to
		// node one.
		//
		uint32_t packetSize = 1024;
		uint32_t maxPacketCount = 50;
		Time interPacketInterval = Seconds (0.1);
		for (unsigned int i = 0; i < nWifi; i++){
			std::cout << "i= " << i << std::endl;
			UdpEchoClientHelper client (csmaInterfaces.GetAddress (nCsma-1), Chattyport);
			client.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
			client.SetAttribute ("Interval", TimeValue (interPacketInterval));
			client.SetAttribute ("PacketSize", UintegerValue (packetSize));
			ApplicationContainer UdpClientapps = client.Install (wifiStaNodes.Get (i));
			UdpClientapps.Start (Seconds (0.0));
			UdpClientapps.Stop (Seconds (10.0));
		}

	uint16_t port = 1044;  // well-known echo port number
	ApplicationContainer sinkApps,sA;
	std::cout << "Maxbytes " << maxBytes << std::endl;
	for (unsigned int i = 0; i < nWifi; i++){
		std::cout << "i= " << i << std::endl;
		BulkSendHelper source ("ns3::TcpSocketFactory",
				InetSocketAddress (wifiInterfaces.GetAddress (i), port));
		// Set the amount of data to send in bytes.  Zero is unlimited.
		source.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
		ApplicationContainer sourceApps = source.Install (csmaNodes.Get (nCsma-1));
		sourceApps.Start (Seconds (10.0));
		sourceApps.Stop (Seconds (150.0));

		//
		// Create a PacketSinkApplication and install it on node 1
		//
		PacketSinkHelper sink ("ns3::TcpSocketFactory",
				InetSocketAddress (Ipv4Address::GetAny (), port));
		sinkApps = sink.Install (wifiStaNodes.Get (i));
		sA.Add(sinkApps);
		sinkApps.Start (Seconds (10.0));
		sinkApps.Stop (Seconds (150.0));
	}



	if (tracing)
	{
		AsciiTraceHelper ascii;
		csma.EnableAsciiAll (ascii.CreateFileStream ("analysis/udp-chatty-send-aarf.tr"));
		csma.EnablePcapAll ("analysis/udp-chatty-send-aarf", false);
	}



	Ipv4GlobalRoutingHelper::PopulateRoutingTables ();


	//phy.EnablePcap ("third", apDevices.Get (0));
	//csma.EnablePcap ("third", csmaDevices.Get (1), true);
	NS_LOG_INFO ("Run Simulation.");
	Simulator::Stop (Seconds (150.0));
	Simulator::Run ();
	Simulator::Destroy ();
	Ptr<PacketSink> sink1;
	for (unsigned int i = 0; i < nWifi; i++){
		sink1 = DynamicCast<PacketSink> (sA.Get (i));
		std::cout << "Total Bytes Received By : " << i<<" = "<<sink1->GetTotalRx ()<<std::endl;
	}
	return 0;
}
