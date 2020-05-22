/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016 Sébastien Deronne
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
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "ns3/command-line.h"
#include "ns3/core-module.h"
#include "ns3/config.h"
#include "ns3/uinteger.h"
#include "ns3/boolean.h"
#include "ns3/string.h"
#include "ns3/log.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/applications-module.h"
#include "ns3/ssid.h"
#include "ns3/mobility-helper.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/udp-client-server-helper.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/wifi-net-device.h"
#include "ns3/wifi-mac.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/config-store.h"
#include "ns3/internet-module.h"
#include "ns3/packet.h"
#include <iostream>
#include <vector>
#include <math.h>
#include <string>
#include <fstream>
#include <string>
#include <sstream>
#include <ctime>
#include <iomanip>
#include <sys/stat.h>



// This is an example that illustrates how 802.11n aggregation is configured.
// It defines 4 independent Wi-Fi networks (working on different channels).
// Each network contains one access point and one station. Each station
// continuously transmits data packets to its respective AP.
//
// Network topology (numbers in parentheses are channel numbers):
//
//  Network A (36)   Network B (40)   Network C (44)   Network D (48)
//   *      *          *      *         *      *          *      *
//   |      |          |      |         |      |          |      |
//  AP A   STA A      AP B   STA B     AP C   STA C      AP D   STA D
//
// The aggregation parameters are configured differently on the 4 stations:
// - station A uses default aggregation parameter values (A-MSDU disabled, A-MPDU enabled with maximum size of 65 kB);
// - station B doesn't use aggregation (both A-MPDU and A-MSDU are disabled);
// - station C enables A-MSDU (with maximum size of 8 kB) but disables A-MPDU;
// - station D uses two-level aggregation (A-MPDU with maximum size of 32 kB and A-MSDU with maximum size of 4 kB).
//
// Packets in this simulation aren't marked with a QosTag so they
// are considered belonging to BestEffort Access Class (AC_BE).
//
// The user can select the distance between the stations and the APs and can enable/disable the RTS/CTS mechanism.
// Example: ./waf --run "wifi-aggregation --distance=10 --enableRts=0 --simulationTime=20"
//
// The output prints the throughput measured for the 4 cases/networks described above. When default aggregation parameters are enabled, the
// maximum A-MPDU size is 65 kB and the throughput is maximal. When aggregation is disabled, the throughput is about the half of the
// physical bitrate as in legacy wifi networks. When only A-MSDU is enabled, the throughput is increased but is not maximal, since the maximum
// A-MSDU size is limited to 7935 bytes (whereas the maximum A-MPDU size is limited to 65535 bytes). When A-MSDU and A-MPDU are both enabled
// (= two-level aggregation), the throughput is slightly smaller than the first scenario since we set a smaller maximum A-MPDU size.
//
// When the distance is increased, the frame error rate gets higher, and the output shows how it affects the throughput for the 4 networks.
// Even through A-MSDU has less overheads than A-MPDU, A-MSDU is less robust against transmission errors than A-MPDU. When the distance is
// augmented, the throughput for the third scenario is more affected than the throughput obtained in other networks.

using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE ("SimpleMpduAggregation");

vector<string> splitString(string s, char delim);
void installTrafficGenerator(Ptr<ns3::Node> fromNode, Ptr<ns3::Node> toNode, int port, string offeredLoad, int packetSize);
bool fileExists(const std::string& filename);

double simulationTime = 20; //seconds

int main (int argc, char *argv[])
{
  uint32_t payloadSize = 1472; //bytes
 
  
  bool enableRts = 1;
  bool enablePcap = 0;
  bool verifyResults = 0; //used for regression
  int staNum = 1;
  double distance = 3; //meters
  string offeredLoad = "200";
  string outputCsv = "aggregation.csv";

  /////////////////////////////////////////////// CMD PARAMETERS ///////////////////////////////////////////
  /////////////////////////////////////////////// CMD PARAMETERS ///////////////////////////////////////////
  /////////////////////////////////////////////// CMD PARAMETERS ///////////////////////////////////////////
  /////////////////////////////////////////////// CMD PARAMETERS ///////////////////////////////////////////

  CommandLine cmd;
  cmd.AddValue ("payloadSize", "Payload size in bytes", payloadSize);
  cmd.AddValue ("enableRts", "Enable or disable RTS/CTS", enableRts);
  cmd.AddValue ("simulationTime", "Simulation time in seconds", simulationTime);
  cmd.AddValue ("distance", "Distance in meters between the station and the access point", distance);
  cmd.AddValue ("offeredLoad", "Offered Load", offeredLoad);
  cmd.AddValue ("staNum", "Offered Load", staNum);
  cmd.AddValue ("outputCsv", "Offered Load", outputCsv);
  cmd.AddValue ("enablePcap", "Enable/disable pcap file generation", enablePcap);
  cmd.AddValue ("verifyResults", "Enable/disable results verification at the end of the simulation", verifyResults);
  cmd.Parse (argc, argv);

  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", enableRts ? StringValue ("0") : StringValue ("999999"));

  string buffOfferedLoad = offeredLoad;
  offeredLoad = std::to_string(stod(offeredLoad)/(staNum));

  ////////////////////////////////// Stations and AP Containers ////////////////////////////////////////
  ////////////////////////////////// Stations and AP Containers ////////////////////////////////////////
  ////////////////////////////////// Stations and AP Containers ////////////////////////////////////////
  ////////////////////////////////// Stations and AP Containers and NET DEVICE CONTAINERS ////////////////////////////////////////
  

  NodeContainer wifiStaNodesA, wifiStaNodesB, wifiStaNodesC, wifiStaNodesD;
  wifiStaNodesA.Create (staNum);
  wifiStaNodesB.Create (staNum);
  wifiStaNodesC.Create (staNum);
  wifiStaNodesD.Create (staNum);
  NodeContainer wifiApNodes;
  wifiApNodes.Create (4);

  
  NetDeviceContainer staDeviceA , apDeviceA;
  NetDeviceContainer staDeviceB, staDeviceC, staDeviceD, apDeviceB, apDeviceC, apDeviceD;

  ///////////////////////////////////////////////////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////////////////////////////////
  //////////////////////////// SETUP CHANNEL HELPER AND 802.11 Amdendment -->> 802.11ax 5GHZ

 // YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
 // YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
 // phy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);
 // phy.SetChannel (channel.Create ());

  // Configure wireless channel
  YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
  Ptr<YansWifiChannel> channel;
  YansWifiChannelHelper channelHelper = YansWifiChannelHelper::Default ();
  channelHelper.AddPropagationLoss ("ns3::NakagamiPropagationLossModel");
  phy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11_RADIO);
  phy.SetChannel (channelHelper.Create ());
  ////
  WifiHelper wifi;
  wifi.SetStandard (WIFI_PHY_STANDARD_80211ax_5GHZ);
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode", StringValue ("HeMcs11"), "ControlMode", StringValue ("HeMcs0"));
  WifiMacHelper mac;

  

  

  ///////////////////////////////////// NETWORK A ///////////////////////////////////////////////////////
  ///////////////////////////////////// NETWORK A ///////////////////////////////////////////////////////
  ///////////////////////////////////// NETWORK A ///////////////////////////////////////////////////////
  ///////////////////////////////////// NETWORK A ///////////////////////////////////////////////////////
  Ssid ssid;

  ssid = Ssid ("network-A");
  phy.Set ("ChannelNumber", UintegerValue (36));
  mac.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid));
  
  
  staDeviceA = wifi.Install (phy, mac, wifiStaNodesA);
  

  mac.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid),
               "EnableBeaconJitter", BooleanValue (false));

  
  apDeviceA = wifi.Install (phy, mac, wifiApNodes.Get(0));

  ///////////////////////////////////// NETWORK B ///////////////////////////////////////////////////////
  ///////////////////////////////////// NETWORK B ///////////////////////////////////////////////////////
  ///////////////////////////////////// NETWORK B ///////////////////////////////////////////////////////
  ///////////////////////////////////// NETWORK B ///////////////////////////////////////////////////////

  ssid = Ssid ("network-B");
  phy.Set ("ChannelNumber", UintegerValue (40));
  mac.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid));

  staDeviceB = wifi.Install (phy, mac, wifiStaNodesB);
  
  // Disable A-MPDU

  Ptr<NetDevice> dev;
  Ptr<WifiNetDevice> wifi_dev;

  for(int i = 0 ; i < staNum; i++){
    dev = wifiStaNodesB.Get (i)->GetDevice (0);
    wifi_dev = DynamicCast<WifiNetDevice> (dev);
    wifi_dev->GetMac ()->SetAttribute ("BE_MaxAmpduSize", UintegerValue (0));
  }

  mac.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid),
               "EnableBeaconJitter", BooleanValue (false));
  
  apDeviceB = wifi.Install (phy, mac, wifiApNodes.Get (1));
  
  // // Disable A-MPDU
  dev = wifiApNodes.Get (1)->GetDevice (0);
  wifi_dev = DynamicCast<WifiNetDevice> (dev);
  wifi_dev->GetMac ()->SetAttribute ("BE_MaxAmpduSize", UintegerValue (0));

  ///////////////////////////////////// NETWORK C ///////////////////////////////////////////////////////
  ///////////////////////////////////// NETWORK C ///////////////////////////////////////////////////////
  ///////////////////////////////////// NETWORK C ///////////////////////////////////////////////////////
  ///////////////////////////////////// NETWORK C ///////////////////////////////////////////////////////

  ssid = Ssid ("network-C");
  phy.Set ("ChannelNumber", UintegerValue (44));
  mac.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid));

  staDeviceC = wifi.Install (phy, mac, wifiStaNodesC);

  // Disable A-MPDU and enable A-MSDU with the highest maximum size allowed by the standard (7935 bytes)
  for(int i = 0 ; i < staNum; i++){
    dev = wifiStaNodesC.Get (i)->GetDevice (0);
    wifi_dev = DynamicCast<WifiNetDevice> (dev);
    wifi_dev->GetMac ()->SetAttribute ("BE_MaxAmpduSize", UintegerValue (0));
    wifi_dev->GetMac ()->SetAttribute ("BE_MaxAmsduSize", UintegerValue (7935));
  }

  mac.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid),
               "EnableBeaconJitter", BooleanValue (false));
  apDeviceC = wifi.Install (phy, mac, wifiApNodes.Get (2));

  // // Disable A-MPDU and enable A-MSDU with the highest maximum size allowed by the standard (7935 bytes)
  dev = wifiApNodes.Get (2)->GetDevice (0);
  wifi_dev = DynamicCast<WifiNetDevice> (dev);
  wifi_dev->GetMac ()->SetAttribute ("BE_MaxAmpduSize", UintegerValue (0));
  wifi_dev->GetMac ()->SetAttribute ("BE_MaxAmsduSize", UintegerValue (7935));

  
  ///////////////////////////////////// NETWORK D ///////////////////////////////////////////////////////
  ///////////////////////////////////// NETWORK D ///////////////////////////////////////////////////////
  ///////////////////////////////////// NETWORK D ///////////////////////////////////////////////////////
  ///////////////////////////////////// NETWORK D ///////////////////////////////////////////////////////


  ssid = Ssid ("network-D");
  phy.Set ("ChannelNumber", UintegerValue (48));
  mac.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid));

  staDeviceD = wifi.Install (phy, mac, wifiStaNodesD);

  // Enable A-MPDU with a smaller size than the default one and
  // enable A-MSDU with the smallest maximum size allowed by the standard (3839 bytes)
  for(int i = 0 ; i < staNum; i++){
    dev = wifiStaNodesD.Get (i)->GetDevice (0);
    wifi_dev = DynamicCast<WifiNetDevice> (dev);
    wifi_dev->GetMac ()->SetAttribute ("BE_MaxAmpduSize", UintegerValue (32768));
    wifi_dev->GetMac ()->SetAttribute ("BE_MaxAmsduSize", UintegerValue (3839));
  }

  mac.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid),
               "EnableBeaconJitter", BooleanValue (false));
  apDeviceD = wifi.Install (phy, mac, wifiApNodes.Get (3));

  // Enable A-MPDU with a smaller size than the default one and
  // enable A-MSDU with the smallest maximum size allowed by the standard (3839 bytes)
  dev = wifiApNodes.Get (3)->GetDevice (0);
  wifi_dev = DynamicCast<WifiNetDevice> (dev);
  wifi_dev->GetMac ()->SetAttribute ("BE_MaxAmpduSize", UintegerValue (32768));
  wifi_dev->GetMac ()->SetAttribute ("BE_MaxAmsduSize", UintegerValue (3839));

  //////////////////////////////////////////////// Allocate Stations and AP //////////////////////////////////////
  //////////////////////////////////////////////// //////////////////////// //////////////////////////////////////
   //////////////////////////////////////////////// Allocate Stations and AP //////////////////////////////////////
  //////////////////////////////////////////////// //////////////////////// //////////////////////////////////////


  // Setting mobility model
  MobilityHelper mobility;
  Ptr<ListPositionAllocator> positionAllocA = CreateObject<ListPositionAllocator> ();
  Ptr<ListPositionAllocator> positionAllocB = CreateObject<ListPositionAllocator> ();
  Ptr<ListPositionAllocator> positionAllocC = CreateObject<ListPositionAllocator> ();
  Ptr<ListPositionAllocator> positionAllocD = CreateObject<ListPositionAllocator> ();
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");

  // Set position for APs
  positionAllocA->Add (Vector (0.0, 0.0, 0.0));
  positionAllocB->Add (Vector (100.0, 0.0, 0.0));
  positionAllocC->Add (Vector (200.0, 0.0, 0.0));
  positionAllocD->Add (Vector (300.0, 0.0, 0.0));
  // Set position for STAs
  for (int i = 0 ; i < staNum; i++){
    positionAllocA->Add (Vector (distance, 0.0, 0.0));
  }
  for (int i = 0 ; i < staNum; i++){
    positionAllocB->Add (Vector (distance + 100, 0.0, 0.0));
  }
   for (int i = 0 ; i < staNum; i++){
    positionAllocC->Add (Vector (distance + 200, 0.0, 0.0));
  }
  for (int i = 0 ; i < staNum; i++){
    positionAllocD->Add (Vector (distance + 300, 0.0, 0.0));
  }


  mobility.SetPositionAllocator (positionAllocA);
  mobility.Install (wifiApNodes.Get(0));
  mobility.Install (wifiStaNodesA);
  
  mobility.SetPositionAllocator (positionAllocB);
  mobility.Install (wifiApNodes.Get(1));
  mobility.Install (wifiStaNodesB);
  
  mobility.SetPositionAllocator (positionAllocC);
  mobility.Install (wifiApNodes.Get(2));
  mobility.Install (wifiStaNodesC);
  
  mobility.SetPositionAllocator (positionAllocD);
  mobility.Install (wifiApNodes.Get(3));
  mobility.Install (wifiStaNodesD);
 


  // Internet stack
////////////////////////////////////////// Assign addresses for each network ///////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////// Assign addresses for each network ///////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////


  InternetStackHelper stack;
  stack.Install (wifiApNodes);
  stack.Install (wifiStaNodesA);
  stack.Install (wifiStaNodesB);
  stack.Install (wifiStaNodesC);
  stack.Install (wifiStaNodesD);
   
  Ipv4InterfaceContainer StaInterfaceA;
  Ipv4InterfaceContainer ApInterfaceA;
  
  Ipv4InterfaceContainer StaInterfaceB;
  Ipv4InterfaceContainer ApInterfaceB;
  
  Ipv4InterfaceContainer StaInterfaceC;
  Ipv4InterfaceContainer ApInterfaceC;
  
  Ipv4InterfaceContainer StaInterfaceD;
  Ipv4InterfaceContainer ApInterfaceD;

  Ipv4AddressHelper addressA;
  addressA.SetBase ("192.168.1.0", "255.255.255.0");  // ssid = Ssid ("network-A");
  ApInterfaceA = addressA.Assign (apDeviceA);
  StaInterfaceA  = addressA.Assign (staDeviceA);

  Ipv4AddressHelper addressB;
  addressB.SetBase ("192.168.2.0", "255.255.255.0"); // ssid = Ssid ("network-B");
  ApInterfaceB = addressB.Assign (apDeviceB);
  StaInterfaceB  = addressB.Assign (staDeviceB);

    Ipv4AddressHelper addressC;
  addressC.SetBase ("192.168.3.0", "255.255.255.0"); // ssid = Ssid ("network-C");
  ApInterfaceC = addressC.Assign (apDeviceC);
  StaInterfaceC  = addressC.Assign (staDeviceC);

  Ipv4AddressHelper addressD;
  addressD.SetBase ("192.168.4.0", "255.255.255.0"); // ssid = Ssid ("network-D");
  ApInterfaceD = addressD.Assign (apDeviceD);
  StaInterfaceD  = addressD.Assign (staDeviceD);



  /////////////////////////////// Generate traffic for each network ////////////////////////////////////////
  /////////////////////////////// ///////////////////////////////// ////////////////////////////////////////
  int port=9;
	for(int i = 0; i < staNum; ++i) {
    
		installTrafficGenerator(wifiStaNodesA.Get(i),wifiApNodes.Get(0), port++, offeredLoad, payloadSize);
    installTrafficGenerator(wifiStaNodesB.Get(i),wifiApNodes.Get(1), port++, offeredLoad, payloadSize);
    installTrafficGenerator(wifiStaNodesC.Get(i),wifiApNodes.Get(2), port++, offeredLoad, payloadSize);
    installTrafficGenerator(wifiStaNodesD.Get(i),wifiApNodes.Get(3), port++, offeredLoad, payloadSize);
	}

  ///////////////////////// FLOW MONITOR //////////////////////////////////////////////////
  ///////////////////////// FLOW MONITOR //////////////////////////////////////////////////
  ///////////////////////// FLOW MONITOR //////////////////////////////////////////////////
  ///////////////////////// FLOW MONITOR //////////////////////////////////////////////////

  FlowMonitorHelper flowmon;
	Ptr<FlowMonitor> monitor = flowmon.InstallAll ();
	monitor->SetAttribute ("StartTime", TimeValue (Seconds (0)));

  double flowThr;
	double flowDel;

 if (enablePcap){
  phy.EnablePcap ("AP_A", apDeviceA.Get(0));
  phy.EnablePcap ("AP_B", apDeviceA.Get(1));
  phy.EnablePcap ("AP_C", apDeviceA.Get(2));
  phy.EnablePcap ("AP_D", apDeviceA.Get(3));
  phy.EnablePcap ("STA_A", staDeviceA);
  phy.EnablePcap ("STA_B", staDeviceB);
  phy.EnablePcap ("STA_C", staDeviceC);
  phy.EnablePcap ("STA_D", staDeviceD);
 }
  Simulator::Stop (Seconds (simulationTime + 1));
  Simulator::Run ();


	ofstream myfile;
	if (fileExists(outputCsv))
	{
		myfile.open (outputCsv, ios::app);
	}
	else {
		myfile.open (outputCsv, ios::app);  
		myfile << "OfferedLoad,TopologyNumber,nSta,RngRun,Distance,PayloadSize,SourceIP,DestinationIP,Throughput,Delay,RtsCts" << std::endl;
	}
  
	//Get timestamp
	//auto t = std::time(nullptr);
	//auto tm = *std::localtime(&t);

	Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
	std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats ();
	for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i) {
		Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
		flowThr=i->second.rxPackets * payloadSize * 8.0 / (i->second.timeLastRxPacket.GetSeconds () - i->second.timeFirstTxPacket.GetSeconds ()) / 1000000;
		flowDel=i->second.delaySum.GetSeconds () / i->second.rxPackets;

    
    ostringstream out;
    t.sourceAddress.Print( out );
    string s_res = out.str();
    auto vec = splitString(s_res,'.');
    string net_version = "";
    
    switch(stoi(vec[2])){
      case 1:
        net_version = "A";
        break;
      case 2:
        net_version = "B";
        break;
      case 3:
        net_version = "C";
        break;
      case 4:
        net_version = "D";
        break;
    }
		NS_LOG_UNCOND ("Flow " << i->first  << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\tThroughput: " <<  flowThr  << " Mbps\tTime: " << i->second.timeLastRxPacket.GetSeconds () - i->second.timeFirstTxPacket.GetSeconds () << "\tDelay: " << flowDel << " s\tTx packets " << i->second.txPackets << " s\tRx packets " << i->second.rxPackets << "\n");
		myfile << buffOfferedLoad << "," << net_version << "," << staNum << "," << RngSeedManager::GetRun() << "," << distance << "," << payloadSize << "," << t.sourceAddress << "," << t.destinationAddress << "," << flowThr << "," << flowDel << "," << enableRts;
		myfile << std::endl;
	}
	myfile.close();
	
	
	/* End of simulation */
	Simulator::Destroy ();
  

  return 0;
}
//////////////////////////////////////////// HELPER FUNCTIONS //////////////////////////////////////////////////////
//////////////////////////////////////////// HELPER FUNCTIONS //////////////////////////////////////////////////////
//////////////////////////////////////////// HELPER FUNCTIONS //////////////////////////////////////////////////////
//////////////////////////////////////////// HELPER FUNCTIONS //////////////////////////////////////////////////////

void installTrafficGenerator(Ptr<ns3::Node> fromNode, Ptr<ns3::Node> toNode, int port, string offeredLoad, int packetSize) {

	Ptr<Ipv4> ipv4 = toNode->GetObject<Ipv4> ();
	Ipv4Address addr = ipv4->GetAddress (1, 0).GetLocal ();
  //NS_LOG_UNCOND(addr);
  //addr.Set("255.255.255.255");

	ApplicationContainer sourceApplications, sinkApplications;

	uint8_t tosValue = 0x70;

	//Add random fuzz to app start time
	double min = 0.0;
	double max = 1.0;
	Ptr<UniformRandomVariable> fuzz = CreateObject<UniformRandomVariable> ();
	fuzz->SetAttribute ("Min", DoubleValue (min));
	fuzz->SetAttribute ("Max", DoubleValue (max));

	InetSocketAddress sinkSocket (addr, port);
	sinkSocket.SetTos (tosValue);
	//OnOffHelper onOffHelper ("ns3::TcpSocketFactory", sinkSocket);
	OnOffHelper onOffHelper ("ns3::UdpSocketFactory", sinkSocket);
	onOffHelper.SetConstantRate (DataRate (offeredLoad + "Mbps"), packetSize-20-8-8);
	sourceApplications.Add (onOffHelper.Install (fromNode)); //fromNode


	//PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", sinkSocket);
	PacketSinkHelper packetSinkHelper ("ns3::UdpSocketFactory", sinkSocket);
	sinkApplications.Add (packetSinkHelper.Install (toNode)); //toNode

	sinkApplications.Start (Seconds (0.0));
	sinkApplications.Stop (Seconds (simulationTime + 1));
	sourceApplications.Start (Seconds (1.0+(roundf(fuzz->GetValue ()*1000)/1000)));
	sourceApplications.Stop (Seconds (simulationTime + 1));

}
bool fileExists(const std::string& filename)
{
    struct stat buf;
    if (stat(filename.c_str(), &buf) != -1)
    {
        return true;
    }
    return false;
}

double **calculateSTApositions(double x_ap, double y_ap, int h, int n_stations) {

	double PI  =3.141592653589793238463;


	double tab[2][n_stations];
	double** sta_co=0;
	sta_co = new double*[2];
	sta_co[0]=new double[n_stations];
	sta_co[1]=new double[n_stations];
	double ANG = 2*PI;

	float X=1;
	for(int i=0; i<n_stations; i++){
		float sta_x = static_cast <float> (rand()) / (static_cast <float> (RAND_MAX/X));
		tab[0][i]= sta_x*h;

	}

	for (int j=0; j<n_stations; j++){
		float angle = static_cast <float> (rand()) / (static_cast <float> (RAND_MAX/ANG));
		tab[1][j]=angle;

	}
	for ( int k=0; k<n_stations; k++){
		sta_co[0][k]=x_ap+cos(tab[1][k])*tab[0][k];
		sta_co[1][k]=y_ap+sin(tab[1][k])*tab[0][k];

	}
	return sta_co;
}
vector<string> splitString(string s, char delim)
{
  vector<string> res;
  stringstream data(s);
  string line;

  while(getline(data,line,delim))
  {
    res.push_back(line);
  }
  return res;
}