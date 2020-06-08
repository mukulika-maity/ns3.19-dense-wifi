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
	#include "ns3/config-store-module.h"
	#include "ns3/ipv4-list-routing-helper.h"
	#include "ns3/point-to-point-module.h"

	#include "ns3/http-module.h"

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
		uint32_t nWifi = 5;
		// uint32_t maxBytes = 1048576;
		bool tracing = true;
		// uint32_t totalTime = 100;
		//uint32_t dataStart = 0;
		Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (1448));
		CommandLine cmd;
		cmd.AddValue ("nCsma", "Number of \"extra\" CSMA nodes/devices", nCsma);
		cmd.AddValue ("nWifi", "Number of wifi STA devices", nWifi);
		cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);

		cmd.Parse (argc,argv);

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
		uint16_t port = 80;  // well-known echo port number
		HttpHelper httpHelper;

		HttpServerHelper httpServer;
		NS_LOG_LOGIC ("Install app in server");
		/*HttpController httpCtrl;
				 httpCtrl=httpHelper.GetController ();*/
		//httpHelper.SetAttribute ("SegmentSize", UintegerValue (60));
		httpServer.SetAttribute ("Local", AddressValue (InetSocketAddress (Ipv4Address::GetAny (), port)));
		httpServer.SetAttribute ("HttpController", PointerValue (httpHelper.GetController ()));
		httpServer.SetAttribute ("Persistent", BooleanValue (false));
		ApplicationContainer serverApps = httpServer.Install (csmaNodes.Get (nCsma-1));

		serverApps.Start (Seconds (0.0));
		serverApps.Stop (Seconds (100.0));
		for (unsigned int i = 0; i < nWifi; i++){

			HttpClientHelper httpClient;
			httpClient.SetAttribute ("Peer", AddressValue (InetSocketAddress (csmaInterfaces.GetAddress (nCsma-1), port)));
			httpClient.SetAttribute ("HttpController", PointerValue (httpHelper.GetController ()));
			httpClient.SetAttribute ("Internet",BooleanValue (false));
			httpClient.SetAttribute ("MaxSessions", UintegerValue (1));
			/*
			 * No need to define if using automatic traffic generation mode
			 */
			httpClient.SetAttribute ("UserNumPages", UintegerValue (1));
			httpClient.SetAttribute ("UserNumObjects", UintegerValue (1));
			httpClient.SetAttribute ("UserServerDelay", DoubleValue (0.1));
			httpClient.SetAttribute ("UserPageRequestGap", DoubleValue (0.1));
			httpClient.SetAttribute ("UserRequestSize", UintegerValue (1460));
			httpClient.SetAttribute ("UserResponseSize", UintegerValue (1460));
			/*
			 * This controls the mode, when using http 1.1 and set the ForcePersistent as false, it will
			 * determine the connection type by itself
			 */
			httpClient.SetAttribute ("Persistent", BooleanValue (false));

			ApplicationContainer clientApps = httpClient.Install (wifiStaNodes.Get (i));
			std::cout << "i= " << i << std::endl;
			UniformVariable var;
			clientApps.Start (Seconds (i+1));
			//clientApps.Stop (Seconds (100.0));
		}

		if (tracing)
		{
			AsciiTraceHelper ascii;
			csma.EnableAsciiAll (ascii.CreateFileStream ("tcp-bulk-send.tr"));
			csma.EnablePcapAll ("tcp-bulk-send", false);
		}



		Ipv4GlobalRoutingHelper::PopulateRoutingTables ();



		NS_LOG_INFO ("Run Simulation.");
		Simulator::Stop (Seconds (100.0));
		Simulator::Run ();
		Simulator::Destroy ();

		return 0;
	}
