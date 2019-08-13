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
    // [2019-08-07] TODO: Perform proper generation as specified in 3GPP TS 38.212

    std::size_t totalBytes = m_pdu->getByteLength ();
    assert (totalBytes != 0);

    const std::size_t availableNumCBGs[] = { 8, 6, 4, 2, 1};
    std::size_t numCBGsIdx = m_bCBGEnabled ? 0 : 4;
    std::size_t cbgSize;

    // check which number of CBGs is suitable depending on the total pdu size
    while ((cbgSize = int (ceil (float (totalBytes) / availableNumCBGs [numCBGsIdx]))) == 0)
        numCBGsIdx++;

    std::size_t numCBGs = availableNumCBGs [numCBGsIdx];
    while (numCBGs > 0 && totalBytes > 0)
    {
        std::size_t bytes = (totalBytes >= cbgSize) ? cbgSize : totalBytes;

        NRCodeBlockGroup *cbg = new NRCodeBlockGroup ("NRCodeBlockGroup");
        cbg->setTransportBlock (this);
        cbg->setByteLength (bytes);

        m_vCodeBlockGroups.push_back (std::make_pair (cbg, CBG_STATE_UNKNOWN));

        totalBytes -= bytes;
        numCBGs--;
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
