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

#define CBG_STATE_CORRECT true
#define CBG_STATE_UNKNOWN false

class LteMacTransportBlock
{
public:
    LteMacTransportBlock(bool bCBGEnabled = false);
    LteMacTransportBlock(LteMacPdu * pPdu, bool bCBGEnabled = false);
    ~LteMacTransportBlock();

public:
    LteMacPdu * getPdu ();
    void        setPdu (LteMacPdu * pPdu);

    LteMacPdu * extractPdu ();

    std::size_t getNumCBGs ();
    NRCodeBlockGroup * getCBG (std::size_t idx);

    std::size_t getRemainingBytes ();
    void        setCbgState (std::size_t idx, bool state = CBG_STATE_CORRECT);
    void        setAllState (bool state = CBG_STATE_CORRECT);

private:
    void do_generateCodeBlockGroups ();

    using CBGStatus = std::pair <NRCodeBlockGroup*, bool>;

    std::unique_ptr <LteMacPdu> m_pdu;
    std::vector <CBGStatus>     m_vCodeBlockGroups;
    bool                        m_bCBGEnabled;
};

#endif /* STACK_MAC_PACKET_LTEMACTRANSPORTBLOCK_H_ */
