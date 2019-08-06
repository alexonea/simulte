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

#ifndef STACK_MAC_PACKET_LTEMACTRANSPORTBLOCK_H_
#define STACK_MAC_PACKET_LTEMACTRANSPORTBLOCK_H_

#include <memory>
#include <vector>

#include "LteMacPdu.h"

class NRCodeBlockGroup;

class LteMacTransportBlock
{
public:
    LteMacTransportBlock();
    LteMacTransportBlock(LteMacPdu * pPdu);
    ~LteMacTransportBlock();

public:
    LteMacPdu * getPdu ();
    void        setPdu (LteMacPdu * pPdu);

    LteMacPdu * extractPdu ();

    bool        isCorrect  ();
    void        setCorrect ();

    std::size_t getNumCBGs ();

    void        addCBG (NRCodeBlockGroup *cbg);
    NRCodeBlockGroup * getCBG (std::size_t idx);

private:
    std::size_t                      m_nCodeBlockGroups;
    std::unique_ptr <LteMacPdu>      m_pdu;
    std::vector <NRCodeBlockGroup *> m_vCodeBlockGroups;
    bool                             m_bCorrect;
};

#endif /* STACK_MAC_PACKET_LTEMACTRANSPORTBLOCK_H_ */
