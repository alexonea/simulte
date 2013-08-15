//
//                           SimuLTE
// Copyright (C) 2012 Antonio Virdis, Daniele Migliorini, Giovanni
// Accongiagioco, Generoso Pagano, Vincenzo Pii.
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef __SIMULTE_IP2LTE_H_
#define __SIMULTE_IP2LTE_H_

#include <omnetpp.h>
#include "LteControlInfo.h"
#include "LteControlInfo.h"
#include "IPv4Datagram.h"


/**
 *
 */
class IP2lte : public cSimpleModule
{
    cGate *stackGateOut_;           /// gate connecting LteIp module to LTE stack
    cGate *ipGateOut_;
    LteNodeType nodeType_;     /// node type: can be INTERNET, ENODEB, UE

    unsigned int seqNum_;     /// datagram sequence number (RLC fragmentation needs it)

    /**
     * Handle packets from transport layer and forward them
     * to the specified output gate, after encapsulation in
     * IP Datagram.
     * This method adds to the datagram the LteStackControlInfo.
     *
     * @param transportPacket transport packet received from transport layer
     * @param outputgate output gate where the datagram will be sent
     */
    void fromIpUe(IPv4Datagram * datagram);

    /**
     * Manage packets received from Lte Stack or LteIp peer
     * and forward them to transport layer.
     *
     * @param msg  IP Datagram received from Lte Stack or LteIp peer
     */
    void toIpUe(IPv4Datagram *datagram);

    void fromIpEnb(IPv4Datagram * datagram);
    void toIpEnb(cMessage * msg);

    /**
     * utility: set nodeType_ field
     *
     * @param s string containing the node type ("internet", "enodeb", "ue")
     */
    void setNodeType(std::string s);

    /**
     * utility: print LteStackControlInfo fields
     *
     * @param ci LteStackControlInfo object
     */
    void printControlInfo(FlowControlInfo* ci);
    void registerInterface();
  protected:
    virtual void initialize();
    virtual void handleMessage(cMessage *msg);

};

#endif
