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
double old_time = 0.0;
EventId output1;

Time currentd = Time::FromDouble(0.01, Time::S);  //Only record cwnd and ssthresh values every 3 seconds
bool first = true;

static void
BuffTracer (Ptr<OutputStreamWrapper>stream,NetDeviceContainer apDevices)
{
	double new_time = Simulator::Now().GetSeconds();
	if (old_time == 0 && first)
	{

		//double mycurrent = current.GetSeconds();
		Ptr<WifiNetDevice> dev = DynamicCast<WifiNetDevice> (apDevices.Get (0));
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

		std::cout<<"buff "<< new_time << " " << queue->GetSize()<< std::endl;
		*stream->GetStream() << new_time << " " << queue->GetSize() << std::endl;
		first = false;
		output1 = Simulator::Schedule(currentd,&BuffTracer,stream,apDevices);
	}
	else
	{
		if (output1.IsExpired())
		{
			Ptr<WifiNetDevice> dev = DynamicCast<WifiNetDevice> (apDevices.Get (0));
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
			std::cout<<"buff "<< new_time << " " << queue->GetSize() << std::endl;
			*stream->GetStream() << new_time << " " << queue->GetSize() << std::endl;
			output1.Cancel();
			output1 = Simulator::Schedule(currentd,&BuffTracer,stream,apDevices);
		}
	}
}

/*void SetCwMin(uint16_t cw)
{
   Config::Set("/NodeList/0/DeviceList/0/$ns3::WifiNetDevice/Mac/$ns3::RegularWifiMac/DcaTxop/CwMin", UintegerValue(cw));
}*/
int 
main (int argc, char *argv[])
{
	bool verbose = true;
	uint32_t nCsma = 2;
	uint32_t nWifi = 30;//15;
	uint32_t maxBytes = 80240;//5242880;//9971233;
	bool tracing = true;
	std::string buff_tr_file_name = "bulk_onoff_buff.tr";
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
	csma.SetChannelAttribute ("Delay", TimeValue (MicroSeconds(500)));

	NetDeviceContainer csmaDevices;
	csmaDevices = csma.Install (csmaNodes);

	NodeContainer wifiStaNodes;
	wifiStaNodes.Create (nWifi);
	NodeContainer wifiApNode = csmaNodes.Get (0);

	YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
	YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
	phy.SetChannel (channel.Create ());

	WifiHelper wifi = WifiHelper::Default ();
	wifi.SetStandard (WIFI_PHY_STANDARD_80211g);
	//wifi.SetRemoteStationManager ("ns3::MinstrelWifiManager");
	wifi.SetRemoteStationManager ("ns3::AarfWifiManager");
	//wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
	  //                                "DataMode", StringValue ("OfdmRate54Mbps"));

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
	Ptr<WifiNetDevice> dev = DynamicCast<WifiNetDevice> (apDevices.Get (0));
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
	queue->SetMaxSize(512);
	std::cout<<"Queue size: " << queue->GetMaxSize()<<std::endl;
	std::cout<<"Contention window size: " << dca->GetMinCw()<<std::endl;
	//dca->SetMinCw(32);
	std::cout<<"Contention window size: " << dca->GetMinCw()<<std::endl;
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
	uint16_t port = 9;  // well-known echo port number
	ApplicationContainer sinkApps,sA;
	std::cout << "Maxbytes " << maxBytes << std::endl;
	for (unsigned int i = 0; i < nWifi; i++){
		int seed = i+1;
		SeedManager::SetSeed (seed);
		SeedManager::SetRun (1);
		UniformVariable var;
		double rand = var.GetValue(0.1,4);
		std::cout<<"var="<<rand<<std::endl;
		std::cout << "i= " << i << std::endl;
		BulkSendHelper source ("ns3::TcpSocketFactory",
				InetSocketAddress (wifiInterfaces.GetAddress (i), port));
		// Set the amount of data to send in bytes.  Zero is unlimited.
		source.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
		ApplicationContainer sourceApps = source.Install (csmaNodes.Get (nCsma-1));
		sourceApps.Start (Seconds (0.1));
		sourceApps.Stop (Seconds (30.0));

		//
		// Create a PacketSinkApplication and install it on node 1
		//
		PacketSinkHelper sink ("ns3::TcpSocketFactory",
				InetSocketAddress (Ipv4Address::GetAny (), port));
		sinkApps = sink.Install (wifiStaNodes.Get (i));
		sA.Add(sinkApps);
		sinkApps.Start (Seconds (0.1));
		sinkApps.Stop (Seconds (30.0));
	}
	/*uint16_t onoffPort = 4000;
	PacketSinkHelper onoffSink ("ns3::TcpSocketFactory",
			InetSocketAddress (Ipv4Address::GetAny (), onoffPort));
	ApplicationContainer onoffsinkApps = onoffSink.Install (csmaNodes.Get (nCsma-1));
	onoffsinkApps.Start (Seconds (0.1));
	onoffsinkApps.Stop (Seconds (1000.0));
	for (unsigned int i = 0; i < nWifi;i++)
	{
		Address remoteAddress (InetSocketAddress (csmaInterfaces.GetAddress (nCsma-1), onoffPort));
		OnOffHelper clientHelper ("ns3::TcpSocketFactory", remoteAddress);
		clientHelper.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
		clientHelper.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
		clientHelper.SetAttribute ("PacketSize", StringValue ("300"));
		clientHelper.SetAttribute ("DataRate", StringValue ("10Kbps"));
		ApplicationContainer clientApp = clientHelper.Install (wifiStaNodes.Get (i));
		clientApp.Start (Seconds (0.1));
		clientApp.Stop (Seconds (1000.0));
	}*/
	if (tracing)
	{
		AsciiTraceHelper ascii;
		csma.EnableAsciiAll (ascii.CreateFileStream ("analysis/traffic_model/sol/sol_minstrel_test3.tr"));
		csma.EnablePcapAll ("analysis/traffic_model/sol/sol_minstrel_test3", false);
		//Ptr<OutputStreamWrapper> stream = ascii.CreateFileStream(buff_tr_file_name.c_str());
		//Simulator::Schedule(Seconds(0.00002), &BuffTracer, stream,apDevices);
	}
	//Simulator::Schedule(Seconds(0.00001), &SetCwMin, 32);
	/*UdpEchoServerHelper echoServer (9);

  ApplicationContainer serverApps = echoServer.Install (csmaNodes.Get (nCsma-1));
  serverApps.Start (Seconds (1.0));
  serverApps.Stop (Seconds (10.0));

  UdpEchoClientHelper echoClient (csmaInterfaces.GetAddress (nCsma-1), 9);
  echoClient.SetAttribute ("MaxPackets", UintegerValue (1));
  echoClient.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
  echoClient.SetAttribute ("PacketSize", UintegerValue (1024));

  ApplicationContainer clientApps = 
    echoClient.Install (wifiStaNodes.Get (nWifi - 1));
  clientApps.Start (Seconds (2.0));
  clientApps.Stop (Seconds (10.0));*/

	Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
	phy.SetPcapDataLinkType(YansWifiPhyHelper::DLT_IEEE802_11_RADIO);

	phy.EnablePcap ("analysis/traffic_model/sol/ap_sol_minstrel_test3", apDevices.Get (0));
	csma.EnablePcap ("third", csmaDevices.Get (1), true);
	NS_LOG_INFO ("Run Simulation.");
	Simulator::Stop (Seconds (30.0));
	Simulator::Run ();
	Simulator::Destroy ();
	Ptr<PacketSink> sink1;
	for (unsigned int i = 0; i < nWifi; i++){
		sink1 = DynamicCast<PacketSink> (sA.Get (i));
		std::cout << "Total Bytes Received By : " << i<<" = "<<sink1->GetTotalRx ()<<std::endl;
	}
	return 0;
}
