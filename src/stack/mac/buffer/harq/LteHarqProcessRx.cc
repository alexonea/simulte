//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "stack/mac/buffer/harq/LteHarqProcessRx.h"
#include "stack/mac/layer/LteMacBase.h"
#include "common/LteControlInfo.h"
#include "stack/mac/packet/LteHarqFeedback_m.h"
#include "stack/mac/packet/LteMacPdu.h"

LteHarqProcessRx::LteHarqProcessRx(unsigned char acid, LteMacBase *owner)
{
    tb_.resize(MAX_CODEWORDS, NULL);
    status_.resize(MAX_CODEWORDS, RXHARQ_PDU_EMPTY);
    rxTime_.resize(MAX_CODEWORDS, 0);
    result_.resize(MAX_CODEWORDS, false);
    acid_ = acid;
    macOwner_ = owner;
    transmissions_ = 0;
    maxHarqRtx_ = owner->par("maxHarqRtx");
}

void LteHarqProcessRx::insertPdu(Codeword cw, NRMacPacket *macPkt)
{
    UserControlInfo *lteInfo = check_and_cast<UserControlInfo *>(macPkt->getControlInfo());
    bool ndi = lteInfo->getNdi();

    EV << "LteHarqProcessRx::insertPdu - ndi is " << ndi << endl;
    if (ndi && !(status_.at(cw) == RXHARQ_PDU_EMPTY))
        throw cRuntimeError("New data arriving in busy harq process -- this should not happen");

    if (!ndi && !(status_.at(cw) == RXHARQ_PDU_EMPTY) && !(status_.at(cw) == RXHARQ_PDU_CORRUPTED))
        throw cRuntimeError(
            "Trying to insert macPdu in non-empty rx harq process: Node %d acid %d, codeword %d, ndi %d, status %d",
            macOwner_->getMacNodeId(), acid_, cw, ndi, status_.at(cw));

    // deallocate corrupted pdu received in previous transmissions
    if (tb_.at(cw) != NULL && status_.at(cw) != RXHARQ_PDU_EVALUATING){
            macOwner_->dropObj(tb_.at(cw)->getPdu());
            // delete tb_.at(cw);
    } else if (status_.at(cw) == RXHARQ_PDU_EVALUATING) return;

    // [2019-08-06] TODO: Currently, we decide which CBG is corrupt at the transmitting side. It might be possible
    //     to do it here instead, where the decider result is collected. In fact, it might be possible to get the
    //     result directly per-CBG from the decider (much better?), especially in the case of URLLC traffic.

    // store new received pdu
    tb_.at(cw) = macPkt->getTransportBlock ();
    result_.at(cw) = lteInfo->getDeciderResult();
    status_.at(cw) = RXHARQ_PDU_EVALUATING;
    rxTime_.at(cw) = NOW;

    transmissions_++;
}

bool LteHarqProcessRx::isEvaluated(Codeword cw)
{
    if (status_.at(cw) == RXHARQ_PDU_EVALUATING && (NOW - rxTime_.at(cw)) >= HARQ_FB_EVALUATION_INTERVAL)
        return true;
    else
        return false;
}

LteHarqFeedback *LteHarqProcessRx::createFeedback(Codeword cw)
{
    if (!isEvaluated(cw))
        throw cRuntimeError("Cannot send feedback for a pdu not in EVALUATING state");

    UserControlInfo *pduInfo = check_and_cast<UserControlInfo *>(tb_.at(cw)->getPdu()->getControlInfo());
    LteHarqFeedback *fb = new LteHarqFeedback();
    fb->setAcid(acid_);
    fb->setCw(cw);
    fb->setResult(result_.at(cw));
    fb->setFbMacPduId(tb_.at(cw)->getPdu()->getMacPduId());
    fb->setByteLength(0);
    UserControlInfo *fbInfo = new UserControlInfo();
    fbInfo->setSourceId(pduInfo->getDestId());
    fbInfo->setDestId(pduInfo->getSourceId());
    fbInfo->setFrameType(HARQPKT);
    fb->setControlInfo(fbInfo);

    if (!result_.at(cw))
    {
        // NACK will be sent
        status_.at(cw) = RXHARQ_PDU_CORRUPTED;

        EV << "LteHarqProcessRx::createFeedback - tx number " << (unsigned int)transmissions_ << endl;
        if (transmissions_ == (maxHarqRtx_ + 1))
        {
            EV << NOW << " LteHarqProcessRx::createFeedback - max number of tx reached for cw " << cw << ". Resetting cw" << endl;

            // purge PDU
            purgeCorruptedPdu(cw);
            resetCodeword(cw);
        }
    }
    else
    {
        status_.at(cw) = RXHARQ_PDU_CORRECT;
    }

    return fb;
}

bool LteHarqProcessRx::isCorrect(Codeword cw)
{
    return (status_.at(cw) == RXHARQ_PDU_CORRECT);
}

LteMacPdu* LteHarqProcessRx::extractPdu(Codeword cw)
{
    if (!isCorrect(cw))
        throw cRuntimeError("Cannot extract pdu if the state is not CORRECT");

    // temporary copy of pdu pointer because reset NULLs it, and I need to return it
    LteMacPdu *pdu = tb_.at(cw)->getPdu()->dup();
    tb_.at(cw) = NULL;
    resetCodeword(cw);
    return pdu;
}

int64_t LteHarqProcessRx::getByteLength(Codeword cw)
{
    if (tb_.at(cw) != NULL)
    {
        // [2019-08-10] TODO: report the correct number of bytes (re-transmission only) because this is used
        //     to rescheduled the retransmission.
        return tb_.at(cw)->getRemainingBytes();
    }
    else
        return 0;
}

void LteHarqProcessRx::purgeCorruptedPdu(Codeword cw)
{
    // drop ownership
    // [2019-08-06] TODO: check ownership
    // if (pdu_.at(cw) != NULL)
    //     macOwner_->dropObj(pdu_.at(cw));

    // delete tb_.at(cw);
    tb_.at(cw) = NULL;
}

void LteHarqProcessRx::resetCodeword(Codeword cw)
{
    // drop ownership
    if (tb_.at(cw) != NULL){
        macOwner_->dropObj(tb_.at(cw)->getPdu());
        // delete tb_.at(cw);
    }

    tb_.at(cw) = NULL;
    status_.at(cw) = RXHARQ_PDU_EMPTY;
    rxTime_.at(cw) = 0;
    result_.at(cw) = false;

    transmissions_ = 0;
}

LteHarqProcessRx::~LteHarqProcessRx()
{
    for (unsigned char i = 0; i < MAX_CODEWORDS; ++i)
    {
        if (tb_.at(i) != NULL)
        {
            cObject *mac = macOwner_;
            if (tb_.at(i)->getPdu()->getOwner() == mac)
            {
                // delete tb_.at(i);
            }
            tb_.at(i) = NULL;
        }
    }
}

std::vector<RxUnitStatus>
LteHarqProcessRx::getProcessStatus()
{
    std::vector<RxUnitStatus> ret(MAX_CODEWORDS);

    for (unsigned int j = 0; j < MAX_CODEWORDS; j++)
    {
        ret[j].first = j;
        ret[j].second = getUnitStatus(j);
    }
    return ret;
}
