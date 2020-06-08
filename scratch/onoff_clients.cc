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
#include "ns3/flow-monitor-module.h"

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
	uint32_t nWifi = 30;//15;
	bool tracing = true;
	Config::SetDefault ("ns3::TcpSocket::SegmentSize", UintegerValue (1448));
	Config::SetDefault ("ns3::WifiRemoteStationManager::FragmentationThreshold", StringValue ("2200"));
	   // turn off RTS/CTS for frames below 2200 bytes
    Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", StringValue ("2200"));

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
	//csma.SetChannelAttribute ("Delay", TimeValue (MicroSeconds(350)));
	csma.SetChannelAttribute ("Delay", TimeValue (NanoSeconds (6560)));

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
	//wifi.SetRemoteStationManager ("ns3::AarfWifiManager");
	wifi.SetRemoteStationManager ("ns3::MinstrelWifiManager");
	//wifi.SetRemoteStationManager ("ns3::IdealWifiManager");
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
	//queue->SetMaxSize(512);
	queue->SetMaxSize(512);
	//dca->SetMinCw(31);
	        //dca->SetMaxCw(32);
	        //dca->SetAifsn(1);
	std::cout<<"Queue size: " << queue->GetMaxSize()<<std::endl;
	std::cout<<"Contention window size: " << dca->GetMinCw()<<std::endl;
	//dca->SetMinCw(31);
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

	uint16_t onoffPort = 4000;

	for (unsigned int i = 0; i < nWifi;i++)
	{
		Address remoteAddress (InetSocketAddress (wifiInterfaces.GetAddress (i), onoffPort));
		OnOffHelper clientHelper ("ns3::TcpSocketFactory", remoteAddress);
		clientHelper.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
		clientHelper.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
		clientHelper.SetAttribute ("PacketSize", StringValue ("300"));
		clientHelper.SetAttribute ("DataRate", StringValue ("1024Kbps"));
		ApplicationContainer clientApp = clientHelper.Install (csmaNodes.Get (nCsma-1));
		clientApp.Start (Seconds (0.1));
		clientApp.Stop (Seconds (500.0));

		PacketSinkHelper onoffSink ("ns3::TcpSocketFactory",
					InetSocketAddress (Ipv4Address::GetAny (), onoffPort));
			ApplicationContainer onoffsinkApps = onoffSink.Install (wifiStaNodes.Get (i));
			onoffsinkApps.Start (Seconds (0.1));
			onoffsinkApps.Stop (Seconds (500.0));
	}

	if (tracing)
	{
		csma.EnablePcapAll ("analysis/traffic_model/11g_mod/final_results/skype_1mbps_rr", false);
	}


	Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
	phy.SetPcapDataLinkType(YansWifiPhyHelper::DLT_IEEE802_11_RADIO);
	phy.EnablePcap ("analysis/traffic_model/11g_mod/final_results/client_skype_1mbps_rr", staDevices.Get(4));
	phy.EnablePcap ("analysis/traffic_model/11g_mod/final_results/client_skype_1mbps_rr", staDevices.Get(27));
	phy.EnablePcap ("analysis/traffic_model/11g_mod/final_results/ap_skype_1mbps_rr", apDevices.Get (0));
	NS_LOG_INFO ("Run Simulation.");
	Ptr<FlowMonitor> flowMonitor;
	FlowMonitorHelper flowHelper;
	flowMonitor = flowHelper.InstallAll();
	Simulator::Stop (Seconds (500.0));

	Simulator::Run ();
	flowMonitor->SerializeToXmlFile("analysis/traffic_model/11g_mod/final_results/skype_1mbps.rr.xml", true, true);
	Simulator::Destroy ();
	return 0;
}
