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

LteMacTransportBlock::LteMacTransportBlock ()
: m_nCodeBlockGroups {0}
, m_bCorrect         {false}
{}

LteMacTransportBlock::LteMacTransportBlock (LteMacPdu * pPdu)
: m_nCodeBlockGroups {0}
, m_pdu              {pPdu}
, m_bCorrect         {false}
{}

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
}

std::size_t
LteMacTransportBlock::getNumCBGs ()
{
    return m_vCodeBlockGroups.size ();
}

bool
LteMacTransportBlock::isCorrect ()
{
    return m_bCorrect;
}

void
LteMacTransportBlock::setCorrect ()
{
    m_bCorrect = true;
}

void
LteMacTransportBlock::addCBG (NRCodeBlockGroup *cbg)
{
    m_vCodeBlockGroups.push_back (cbg);
}

NRCodeBlockGroup *
LteMacTransportBlock::getCBG (std::size_t idx)
{
    return m_vCodeBlockGroups.at (idx);
}
