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

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("MyWireless");

int 
main (int argc, char *argv[])
{
	bool verbose = true;
	uint32_t nCsma = 2;
	uint32_t nWifi = 30;//15;
	uint32_t maxBytes = 10240;//9971233;
	uint32_t maxTurns = 100;
	bool tracing = true;
	std::string buff_tr_file_name = "bulk_onoff_buff.tr";
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
	      //  dca->SetMinCw(2);
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
	uint16_t servPort = 50;
	ChattyServerHelper chattyServer("ns3::TcpSocketFactory",
	InetSocketAddress (Ipv4Address::GetAny (), servPort));
	chattyServer.SetAttribute ("MaxBytes", UintegerValue (maxBytes));
	for(unsigned int j=0;j<30;j++){
	ApplicationContainer chattyServerApps = chattyServer.Install (csmaNodes.Get (nCsma-1));
	chattyServerApps.Start (Seconds (0.1));
	chattyServerApps.Stop (Seconds (500.0));
	std::cout<<"client id="<<wifiStaNodes.Get (1)->GetId()<<std::endl;
	for (unsigned int i = 0; i < nWifi;i++)
	    {
	      Address remoteAddress (InetSocketAddress (csmaInterfaces.GetAddress (nCsma-1), servPort));
	      ChattyClientHelper chattyClient ("ns3::TcpSocketFactory",remoteAddress);
	             // Set the amount of data to send in bytes.  Zero is unlimited.
	      chattyClient.SetAttribute ("MaxTurns", UintegerValue (maxTurns));
	              ApplicationContainer chattyClientApps = chattyClient.Install (wifiStaNodes.Get (i));
	              int seed = i+1;
	              SeedManager::SetSeed (seed);
	              SeedManager::SetRun (1);
	              UniformVariable var;
	              double rand = var.GetValue(0.1,4);
	              std::cout<<"var="<<rand<<std::endl;
	              chattyClientApps.Start (Seconds (rand+j*5));
	              chattyClientApps.Stop (Seconds (500.0));
	    }
	}
	if (tracing)
	{
		AsciiTraceHelper ascii;
		//csma.EnableAsciiAll (ascii.CreateFileStream ("analysis/traffic_model/sol/sol_const_t_10s_c10.tr"));
		csma.EnablePcapAll ("analysis/traffic_model/11g_mod/short_traffic_4s_5", false);
		//Ptr<OutputStreamWrapper> stream = ascii.CreateFileStream(buff_tr_file_name.c_str());
		//Simulator::Schedule(Seconds(0.00002), &BuffTracer, stream,apDevices);
	}

	Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
	phy.SetPcapDataLinkType(YansWifiPhyHelper::DLT_IEEE802_11_RADIO);

	phy.EnablePcap ("analysis/traffic_model/11g_mod/ap_short_traffic_4s_5", apDevices.Get (0));
	csma.EnablePcap ("third", csmaDevices.Get (1), true);
	NS_LOG_INFO ("Run Simulation.");
	Simulator::Stop (Seconds (500.0));

	Simulator::Run ();

	Simulator::Destroy ();
	return 0;
}
