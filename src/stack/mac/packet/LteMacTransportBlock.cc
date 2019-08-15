//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#include "LteMacTransportBlock.h"

#include "stack/mac/packet/NRCodeBlockGroup_m.h"

#include "stack/mac/amc/UserTxParams.h"

LteMacTransportBlock::LteMacTransportBlock (bool bCBGEnabled, unsigned int maxNumCBGs)
: m_bCBGEnabled {bCBGEnabled}
, m_nMaxCBGs    {maxNumCBGs}
{
    if (maxNumCBGs != 2 && maxNumCBGs != 4 && maxNumCBGs != 6 && maxNumCBGs != 8)
        throw cRuntimeError ("Invalid value for max number of CBGs");
}

LteMacTransportBlock::LteMacTransportBlock (LteMacPdu * pPdu, bool bCBGEnabled, unsigned int maxNumCBGs)
: m_pdu {pPdu}
, m_bCBGEnabled {bCBGEnabled}
, m_nMaxCBGs    {maxNumCBGs}
{
    if (maxNumCBGs != 2 && maxNumCBGs != 4 && maxNumCBGs != 6 && maxNumCBGs != 8)
        throw cRuntimeError ("Invalid value for max number of CBGs");

    do_generateCodeBlockGroups ();
}

LteMacTransportBlock::~LteMacTransportBlock()
{
    for (auto cbg : m_vCodeBlockGroups)
        delete cbg.first;
}

LteMacPdu *
LteMacTransportBlock::extractPdu ()
{
    return m_pdu.release ();
}

LteMacPdu *
LteMacTransportBlock::getPdu ()
{
    return m_pdu.get ();
}

void
LteMacTransportBlock::setPdu (LteMacPdu * pPdu)
{
    m_pdu.reset (pPdu);
    m_vCodeBlockGroups.resize (0);
    do_generateCodeBlockGroups ();
}

std::size_t
LteMacTransportBlock::getNumCBGs ()
{
    return m_vCodeBlockGroups.size ();
}

NRCodeBlockGroup *
LteMacTransportBlock::getCBG (std::size_t idx)
{
    return m_vCodeBlockGroups.at (idx).first;
}

void
LteMacTransportBlock::do_generateCodeBlockGroups ()
{
    // [2019-08-07] TODO: Perform proper generation as specified in 3GPP TS 38.212 and 38.213 9.1.1

    std::size_t totalBytes = m_pdu->getByteLength ();
    assert (totalBytes != 0);

    std::size_t A = totalBytes * 8;

    const std::size_t availableNumCBGs[] = { 8, 6, 4, 2, 1};
    std::size_t numCBGsIdx = m_bCBGEnabled ? 0 : 4;
    std::size_t cbgSize;
    std::size_t numCBGs;
    std::size_t C;
    double R = ((UserControlInfo *) m_pdu->getControlInfo())->getUserTxParams()->getCwRate(0) / 1024;

    if (! m_bCBGEnabled)
    {
        numCBGs = C = 1;
    }
    else
    {
        std::size_t Kcb = (A < 292 || ((A <= 3824) && (R <= 0.67)) || R <= 0.25) ? 3840 : 8448;

        // [2019-08-13] Correction: here the payload size A + CRC should be considered
        //     For now we only consider the payload size. Also small correction on the "else" branch.
        //     Please see 38.212 5.2.2 for corrections
        if (A <= Kcb)
            C = 1;
        else
            C = ceil ((double) A / Kcb);

        numCBGs = (m_nMaxCBGs < C) ? m_nMaxCBGs : C;
    }

    std::size_t cbSize = ceil (float (totalBytes) / C);

    // Perform CBG generation according to 38.213 9.1.1
    std::size_t middle = C % numCBGs;
    std::size_t cbPerCBGUpToMiddle = ceil ((float) C / numCBGs);
    std::size_t cbPerCBGFromMiddle = floor ((float) C / numCBGs);
    std::size_t cbgSizeUpToMiddle = cbPerCBGUpToMiddle * cbSize;
    std::size_t cbgSizeFromMiddle = cbPerCBGFromMiddle * cbSize;

    for (std::size_t i = 0; i < middle; ++i)
    {
        std::size_t bytes = (totalBytes >= cbgSizeUpToMiddle) ? cbgSizeUpToMiddle : totalBytes;
        NRCodeBlockGroup *cbg = new NRCodeBlockGroup ("NRCodeBlockGroup");
        cbg->setTransportBlock (this);
        cbg->setByteLength (bytes);

        m_vCodeBlockGroups.push_back (std::make_pair (cbg, CBG_STATE_UNKNOWN));

        totalBytes -= bytes;
    }

    for (std::size_t i = middle; i < numCBGs; ++i)
    {
        std::size_t bytes = (totalBytes >= cbgSizeFromMiddle) ? cbgSizeFromMiddle : totalBytes;
        NRCodeBlockGroup *cbg = new NRCodeBlockGroup ("NRCodeBlockGroup");
        cbg->setTransportBlock (this);
        cbg->setByteLength (bytes);

        m_vCodeBlockGroups.push_back (std::make_pair (cbg, CBG_STATE_UNKNOWN));

        totalBytes -= bytes;
    }

    assert (totalBytes == 0);
}

std::size_t
LteMacTransportBlock::getRemainingBytes ()
{
    std::size_t bytes = 0;

    for (const auto cbgStatus : m_vCodeBlockGroups)
        if (cbgStatus.second == CBG_STATE_UNKNOWN)
            bytes += cbgStatus.first->getByteLength ();

    return bytes;
}

void
LteMacTransportBlock::setCbgState (std::size_t idx, bool state)
{
    m_vCodeBlockGroups.at (idx).second = state;
}

void
LteMacTransportBlock::setAllState (bool state)
{
    for (auto & cbg : m_vCodeBlockGroups)
        cbg.second = state;
}
