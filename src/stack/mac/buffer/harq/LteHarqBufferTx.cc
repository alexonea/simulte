//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "stack/mac/buffer/harq/LteHarqBufferTx.h"

LteHarqBufferTx::LteHarqBufferTx(unsigned int numProc, LteMacBase *owner, LteMacBase *dstMac)
{
    numProc_ = numProc;
    macOwner_ = owner;
    nodeId_ = dstMac->getMacNodeId();
    selectedAcid_ = HARQ_NONE;
    processes_ = new std::vector<LteHarqProcessTx *>(numProc);
    numEmptyProc_ = numProc;
    for (unsigned int i = 0; i < numProc_; i++)
    {
        (*processes_)[i] = new LteHarqProcessTx(i, MAX_CODEWORDS, numProc_, macOwner_, dstMac);
    }
}

UnitList LteHarqBufferTx::firstReadyForRtx()
{
    unsigned char oldestProcessAcid = HARQ_NONE;
    simtime_t oldestTxTime = NOW + 1;
    simtime_t currentTxTime = 0;

    for (unsigned int i = 0; i < numProc_; i++)
    {
        if ((*processes_)[i]->hasReadyUnits())
        {
            currentTxTime = (*processes_)[i]->getOldestUnitTxTime();
            if (currentTxTime < oldestTxTime)
            {
                oldestTxTime = currentTxTime;
                oldestProcessAcid = i;
            }
        }
    }
    UnitList ret;
    ret.first = oldestProcessAcid;
    if (oldestProcessAcid != HARQ_NONE)
    {
        ret.second = (*processes_)[oldestProcessAcid]->readyUnitsIds();
    }
    return ret;
}

inet::int64 LteHarqBufferTx::pduLength(unsigned char acid, Codeword cw)
{
    return (*processes_)[acid]->getPduLength(cw);
}

void LteHarqBufferTx::markSelected(UnitList unitIds, unsigned char availableTbs)
{
    if (unitIds.second.size() == 0)
    {
        EV << "H-ARQ TX buffer: markSelected(): empty unitIds list" << endl;
        return;
    }

    if (availableTbs == 0)
    {
        EV << "H-ARQ TX buffer: markSelected(): no available TBs" << endl;
        return;
    }

    unsigned char acid = unitIds.first;
    CwList cwList = unitIds.second;

    // [2019-08-03] TODO: This is not allowed in the current form. CBGs cannot be re-transmitted with any TB
    //     other than the one used for the initial transmission (please reformulate this). Neverhtless, a
    //     jumbo TB will most likely prevent the CBGs to be correctly re-transmitted. An alternative solution
    //     is needed. In the first phase, simply disabling this should work (i.e. we only re-transmit up to
    //     the number of available TBs and no more).
    //
    //     Update: the information above does not actuallty apply to the situation below because the available
    //     TB number below is related to the SU-MIMO case and physical multiplexing on multiple codewords.
    //     Nevertheless, in such cases, combining the TBs does result in problems when dealing with CBGs.
    if (cwList.size() > availableTbs)
    {
        //eg: tx mode siso trying to rtx 2 TBs
        // this is the codeword which will contain the jumbo TB
        Codeword cw = cwList.front();
        cwList.pop_front();
        LteMacPdu *basePdu = (*processes_)[acid]->getPdu(cw);
        while (cwList.size() > 0)
        {
            Codeword cw = cwList.front();
            cwList.pop_front();
            LteMacPdu *guestPdu = (*processes_)[acid]->getPdu(cw);
            while(guestPdu->hasSdu())
            basePdu->pushSdu(guestPdu->popSdu());
            while(guestPdu->hasCe())
            basePdu->pushCe(guestPdu->popCe());
            (*processes_)[acid]->dropPdu(cw);
        }
        (*processes_)[acid]->markSelected(cw);
    }
    else
    {
        CwList::iterator it;
        // all units are marked
        for (it = cwList.begin(); it != cwList.end(); it++)
        {
            (*processes_)[acid]->markSelected(*it);
        }
    }

    selectedAcid_ = acid;

    // user tx params could have changed, modify them
    //    UserControlInfo *uInfo = check_and_cast<UserControlInfo *>(basePdu->getControlInfo());
    // TODO: get amc and modify user tx params
    //uInfo->setTxMode(???)

    // debug output
    EV << "H-ARQ TX: process " << (int)selectedAcid_ << " has been selected for retransmission" << endl;
}

void LteHarqBufferTx::insertPdu(unsigned char acid, Codeword cw, LteMacPdu *pdu)
{
    if (selectedAcid_ == HARQ_NONE)
    {
        // the process has not been used for rtx, or it is the first TB inserted, it must be completely empty
        if (!(*processes_)[acid]->isEmpty())
            throw cRuntimeError("H-ARQ TX buffer: new process selected for tx is not completely empty");
    }

    if (!(*processes_)[acid]->isUnitEmpty(cw))
        throw cRuntimeError("LteHarqBufferTx::insertPdu(): unit is not empty");

    // [2019-08-03] TODO: Generate CBGs based on LteMacPdu and encapsulate them into a TB instance. Attach the
    //     TB instance to the LteMacPdu. Alternatively, propagate the TB instance instead of the LteMacPdu and
    //     reference the LteMacPdu from the TB instance.

    selectedAcid_ = acid;
    numEmptyProc_--;
    (*processes_)[acid]->insertPdu(pdu, cw);

    // debug output
    EV << "H-ARQ TX: new pdu (id " << pdu->getId() << " ) inserted into process " << (int)acid << " "
    "codeword id: " << (int)cw << " "
    "for node with id " << check_and_cast<UserControlInfo *>(pdu->getControlInfo())->getDestId() << endl;
}

UnitList
LteHarqBufferTx::firstAvailable()
{
    UnitList ret;

    unsigned char acid = HARQ_NONE;

    if (selectedAcid_ == HARQ_NONE)
    {
        for (unsigned int i = 0; i < numProc_; i++)
        {
            if ((*processes_)[i]->isEmpty())
            {
                acid = i;
                break;
            }
        }
    }
    else
    {
        acid = selectedAcid_;
    }
    ret.first = acid;

    if (acid != HARQ_NONE)
    {
        // if there is any free process, return empty list
        ret.second = (*processes_)[acid]->emptyUnitsIds();
    }

    return ret;
}

UnitList LteHarqBufferTx::getEmptyUnits(unsigned char acid)
{
    // TODO add multi CW check and retx checks
    UnitList ret;
    ret.first = acid;
    ret.second = (*processes_)[acid]->emptyUnitsIds();
    return ret;
}

void LteHarqBufferTx::receiveHarqFeedback(LteHarqFeedback *fbpkt)
{
    EV << "LteHarqBufferTx::receiveHarqFeedback - start" << endl;

    bool result = fbpkt->getResult();
    HarqAcknowledgment harqResult = result ? HARQACK : HARQNACK;
    Codeword cw = fbpkt->getCw();
    unsigned char acid = fbpkt->getAcid();
    long fbPduId = fbpkt->getFbMacPduId(); // id of the pdu that should receive this fb
    long unitPduId = (*processes_)[acid]->getPduId(cw);

    // After handover or a D2D mode switch, the process nay have been dropped. The received feedback must be ignored.
    if ((*processes_)[acid]->isDropped())
    {
        EV << "H-ARQ TX buffer: received pdu for acid " << (int)acid << ". The corresponding unit has been "
        " reset after handover or a D2D mode switch (the contained pdu was dropped). Ignore feedback." << endl;
        delete fbpkt;
        return;
    }

    if (fbPduId != unitPduId)
    {
        // fb is not for the pdu in this unit, maybe the addressed one was dropped
        EV << "H-ARQ TX buffer: received pdu for acid " << (int)acid << "Codeword " << cw << " not addressed"
        " to the actually contained pdu (maybe it was dropped)" << endl;
        EV << "Received id: " << fbPduId << endl;
        EV << "PDU id: " << unitPduId << endl;
        // todo: comment endsim after tests
        throw cRuntimeError("H-ARQ TX: fb is not for the pdu in this unit, maybe the addressed one was dropped");
    }
    bool reset = (*processes_)[acid]->pduFeedback(harqResult, cw);
    if (reset)
    numEmptyProc_++;

    // debug output
    const char *ack = result ? "ACK" : "NACK";
    EV << "H-ARQ TX: feedback received for process " << (int)acid << " codeword " << (int)cw << ""
    " result is " << ack << endl;

    ASSERT(fbpkt->getOwner() == this->macOwner_);
    delete fbpkt;
}

void LteHarqBufferTx::sendSelectedDown()
{
    if (selectedAcid_ == HARQ_NONE)
    {
        EV << "LteHarqBufferTx::sendSelectedDown - no process selected in H-ARQ buffer, nothing to do" << endl;
        return;
    }

    CwList ul = (*processes_)[selectedAcid_]->selectedUnitsIds();
    CwList::iterator it;
    for (it = ul.begin(); it != ul.end(); it++)
    {
        // [2019-08-03] TODO: instead of extracting the LteMacPdu, we should extract only the CBGs selected for
        //     re-transmission.
        LteMacPdu *pduToSend = (*processes_)[selectedAcid_]->extractPdu(*it);
        macOwner_->sendLowerPackets(pduToSend);

        // debug output
        EV << "\t H-ARQ TX: pdu (id " << pduToSend->getId() << " ) extracted from process " << (int)selectedAcid_ << " "
        "codeword " << (int)*it << " for node with id " <<
        check_and_cast<UserControlInfo *>(pduToSend->getControlInfo())->getDestId() << endl;
    }
    selectedAcid_ = HARQ_NONE;
}

void LteHarqBufferTx::dropProcess(unsigned char acid)
{
    // pdus can be dropped only if the unit is in BUFFERED state.
    CwList ul = (*processes_)[acid]->readyUnitsIds();
    CwList::iterator it;

    for (it = ul.begin(); it != ul.end(); it++)
    {
        (*processes_)[acid]->dropPdu(*it);
    }
    // if a process contains units in BUFFERED state, then all units of this
    // process are either empty or in BUFFERED state (ready).
    numEmptyProc_++;
}

void LteHarqBufferTx::selfNack(unsigned char acid, Codeword cw)
{
    bool reset = false;
    CwList ul = (*processes_)[acid]->readyUnitsIds();
    CwList::iterator it;

    for (it = ul.begin(); it != ul.end(); it++)
    {
        reset = (*processes_)[acid]->selfNack(*it);
    }
    if (reset)
        numEmptyProc_++;
}

void LteHarqBufferTx::forceDropProcess(unsigned char acid)
{
    (*processes_)[acid]->forceDropProcess();
    if (acid == selectedAcid_)
        selectedAcid_ = HARQ_NONE;
    numEmptyProc_++;
}

void LteHarqBufferTx::forceDropUnit(unsigned char acid, Codeword cw)
{
    bool reset = (*processes_)[acid]->forceDropUnit(cw);
    if (reset)
    {
        if (acid == selectedAcid_)
            selectedAcid_ = HARQ_NONE;
        numEmptyProc_++;
    }
}

BufferStatus LteHarqBufferTx::getBufferStatus()
{
    BufferStatus bs(numProc_);
    unsigned int numHarqUnits = 0;
    for (unsigned int i = 0; i < numProc_; i++)
    {
        numHarqUnits = (*processes_)[i]->getNumHarqUnits();
        std::vector<UnitStatus> vus(numHarqUnits);
        vus = (*processes_)[i]->getProcessStatus();
        bs[i] = vus;
    }
    return bs;
}

LteHarqProcessTx *
LteHarqBufferTx::getProcess(unsigned char acid)
{
    try
    {
        return (*processes_).at(acid);
    }
    catch (std::out_of_range & x)
    {
        throw cRuntimeError("LteHarqBufferTx::getProcess(): acid %i out of bounds", int(acid));
    }
}

LteHarqProcessTx *
LteHarqBufferTx::getSelectedProcess()
{
    return getProcess(selectedAcid_);
}

LteHarqBufferTx::~LteHarqBufferTx()
{
    std::vector<LteHarqProcessTx *>::iterator it = processes_->begin();
    for (; it != processes_->end(); ++it)
        delete *it;

    processes_->clear();
    delete processes_;
    processes_ = NULL;
    macOwner_ = NULL;
}

bool LteHarqBufferTx::isInUnitList(unsigned char acid, Codeword cw, UnitList unitIds)
{
    if (acid != unitIds.first)
        return false;

    CwList::iterator it;
    for (it = unitIds.second.begin(); it != unitIds.second.end(); it++)
    {
        if (cw == *it)
        {
            return true;
        }
    }
    return false;
}
