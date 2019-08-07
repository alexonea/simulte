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

LteMacTransportBlock::LteMacTransportBlock ()
{}

LteMacTransportBlock::LteMacTransportBlock (LteMacPdu * pPdu)
: m_pdu              {pPdu}
{
    do_generateCodeBlockGroups ();
}

LteMacTransportBlock::~LteMacTransportBlock()
{}

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
    return m_vCodeBlockGroups.at (idx);
}

void
LteMacTransportBlock::do_generateCodeBlockGroups ()
{
    // [2019-08-07] TODO: Perform proper generation as specified in 3GPP TS 38.212

    std::size_t totalBytes = m_pdu->getByteLength ();
    assert (totalBytes != 0);

    const std::size_t availableNumCBGs[] = { /*8, 6, 4, 2,*/ 1};
    std::size_t numCBGsIdx = 0;
    std::size_t cbgSize;

    // check which number of CBGs is suitable depending on the total pdu size
    while ((cbgSize = (totalBytes / availableNumCBGs [numCBGsIdx])) == 0)
        numCBGsIdx++;

    std::size_t numCBGs = availableNumCBGs [numCBGsIdx];
    while (numCBGs > 0)
    {
        NRCodeBlockGroup *cbg = new NRCodeBlockGroup ("NRCodeBlockGroup");
        cbg->setTransportBlock (this);
        cbg->setByteLength ((totalBytes >= cbgSize) ? cbgSize : totalBytes);

        m_vCodeBlockGroups.push_back (cbg);

        totalBytes -= cbgSize;
        numCBGs--;
    }

    assert (totalBytes == 0);
}
