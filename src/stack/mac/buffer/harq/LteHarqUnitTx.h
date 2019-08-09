//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef _LTE_LTEHARQUNITTX_H_
#define _LTE_LTEHARQUNITTX_H_

#include <omnetpp.h>

#include "stack/mac/packet/LteMacPdu.h"
#include "common/LteControlInfo.h"
#include "common/LteCommon.h"
#include "stack/mac/layer/LteMacBase.h"

#include "stack/mac/packet/LteMacTransportBlock.h"
#include "stack/mac/packet/NRCodeBlockGroup_m.h"
#include "stack/mac/packet/NRMacPacket_m.h"

#include <memory>
#include <vector>

class LteMacBase;

/**
 * An LteHarqUnit is an HARQ mac pdu container,
 * an harqBuffer is made of harq processes which is made of harq units.
 *
 * LteHarqUnit manages transmissions and retransmissions.
 * Contained PDU may be in one of four status:
 *
 *                            IDLE    PDU                    READY
 * TXHARQ_PDU_BUFFERED:        no        present locally        ready for rtx
 * TXHARQ_PDU_WAITING:        no        copy present        not ready for tx
 * TXHARQ_PDU_EMPTY:        yes        not present            not ready for tx
 * TXHARQ_PDU_SELECTED:        no        present                will be tx
 */
class LteHarqUnitTx
{
  protected:

    /// Carried sub-burst
    // LteMacPdu *pdu_;
    std::unique_ptr <LteMacTransportBlock> tb_;

    // [2019-08-08] We are not allowed to re-use any of the already used RNGs
    //     since this will interfere with the results. We create a new instance
    //     for each HARQ TX Unit.
    std::unique_ptr <cExponential> random_;

    /// Omnet ID of the pdu
    long pduId_;

    /// PDU size in bytes
    inet::int64 pduLength_;

    // H-ARQ process identifier
    unsigned char acid_;

    /// H-ARQ codeword identifier
    Codeword cw_;

    /// Number of (re)transmissions for current pdu (N.B.: values are 1,2,3,4)
    std::vector <unsigned char> transmissions_;
    unsigned char overallTransmissions_;

    std::vector <TxHarqPduStatus> status_;

    /// TTI at which the pdu has been transmitted
    simtime_t txTime_;

    // reference to the eNB module
    cModule* nodeB_;

    LteMacBase *macOwner_;
    //used for statistics
    LteMacBase *dstMac_;
    //Maximum number of H-ARQ retransmission
    unsigned int maxHarqRtx_;

    // Statistics

    simsignal_t macCellPacketLoss_;
    simsignal_t macPacketLoss_;
    simsignal_t harqErrorRate_;
    simsignal_t harqErrorRate_1_;
    simsignal_t harqErrorRate_2_;
    simsignal_t harqErrorRate_3_;
    simsignal_t harqErrorRate_4_;

    // D2D Statistics
    simsignal_t macCellPacketLossD2D_;
    simsignal_t macPacketLossD2D_;
    simsignal_t harqErrorRateD2D_;
    simsignal_t harqErrorRateD2D_1_;
    simsignal_t harqErrorRateD2D_2_;
    simsignal_t harqErrorRateD2D_3_;
    simsignal_t harqErrorRateD2D_4_;

  public:
    /**
     * Constructor.
     *
     * @param id unit identifier
     */
    LteHarqUnitTx(unsigned char acid, Codeword cw, LteMacBase *macOwner, LteMacBase *dstMac);

    /**
     * Inserts a pdu in this harq unit.
     *
     * When a new pdu is inserted into an H-ARQ unit, its status is TX_HARQ_PDU_SELECTED,
     * so it will be extracted and sent at this same TTI.
     *
     * @param pdu MacPdu to be inserted
     */
    virtual void insertPdu(LteMacPdu *pdu);

    /**
     * Transition from BUFFERED to SELECTED status: the pdu will be extracted when the
     * buffer will be inspected.
     */
    virtual void markSelected();

    /**
     * Returns the macPdu to be sent and increments transmissions_ counter.
     *
     * The H-ARQ process containing this unit, must call this method in order
     * to extract the pdu the Mac layer will send.
     * Before extraction, control info is updated with transmission counter and ndi.
     */
    virtual LteMacPdu *extractPdu();

    /**
     * Returns the CBGs to be sent and increments transmissions_ counter.
     *
     * The H-ARQ process containing this unit, must call this method in order
     * to extract the pdu the Mac layer will send.
     * Before extraction, control info is updated with transmission counter and ndi.
     */
    virtual std::vector <NRCodeBlockGroup *> extractSelectedCBGs();

    /**
     * Returns the packet to be sent which includes all selected CBGs.
     */
    virtual NRMacPacket * extractMacPacket();

    /**
     * Manages ACK/NACK.
     *
     * @param fb ACK or NACK for this H-ARQ unit
     * @return true if the unit has become empty, false if it is still busy
     */
    virtual bool pduFeedback(HarqAcknowledgment fb);

    /**
     * Tells if this unit is currently managing a pdu or not.
     */
    virtual bool isEmpty();

    /**
     * Tells if the PDU is ready for retransmission (the pdu can then be marked to be sent)
     */
    virtual bool isReady();

    /**
     * Tells if the unit contains at least one CBG in the given state
     */
    virtual bool isAtLeastOneInState(TxHarqPduStatus);

    /**
     * If, after evaluating the pdu, it cannot be retransmitted because there isn't
     * enough frame space, a selfNack can be issued to advance the unit status.
     * This avoids a big pdu that cannot be retransmitted (because the channel changed),
     * to lock an H-ARQ unit indefinitely.
     * Must simulate selection, extraction and nack reception.
     * N.B.: txTime is also updated so firstReadyForRtx returns a different pdu
     *
     * @result true if the unit reset as effect of self nack, false otherwise
     */
    virtual bool selfNack();

    /**
     * Resets unit but throws an error if the unit is not in
     * BUFFERED state.
     */
    virtual void dropPdu();

    virtual void forceDropUnit();

    virtual LteMacPdu *getPdu();

    virtual unsigned char getAcid()
    {
        return acid_;
    }

    virtual Codeword getCodeword()
    {
        return cw_;
    }

    virtual unsigned char getTransmissions()
    {
        // [2019-08-05] TODO: clarify the usage of this. Is a per CBG metric
        //     needed or is the overall metric enough?
        return overallTransmissions_;
    }

    virtual inet::int64 getNextTransmissionLength();

    virtual simtime_t getTxTime()
    {
        return txTime_;
    }

    virtual bool isMarked()
    {
        return (isAtLeastOneInState (TXHARQ_PDU_SELECTED));
    }

    virtual long getMacPduId()
    {
        return pduId_;
    }

    virtual TxHarqPduStatus getStatus()
    {
        // [2019-08-05] TODO: Return a more elaborate status including per-CBG indications. For now, we just return
        //     the status of the first CBG.
        return (isAtLeastOneInState(TXHARQ_PDU_BUFFERED) ? TXHARQ_PDU_BUFFERED :
                isAtLeastOneInState(TXHARQ_PDU_SELECTED) ? TXHARQ_PDU_SELECTED :
                isAtLeastOneInState(TXHARQ_PDU_WAITING) ? TXHARQ_PDU_WAITING :
                TXHARQ_PDU_EMPTY);
    }

    virtual ~LteHarqUnitTx();

  protected:

    virtual void resetUnit();
};

#endif
