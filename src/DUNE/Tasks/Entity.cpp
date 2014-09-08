//***************************************************************************
// Copyright 2007-2014 Universidade do Porto - Faculdade de Engenharia      *
// Laboratório de Sistemas e Tecnologia Subaquática (LSTS)                  *
//***************************************************************************
// This file is part of DUNE: Unified Navigation Environment.               *
//                                                                          *
// Commercial Licence Usage                                                 *
// Licencees holding valid commercial DUNE licences may use this file in    *
// accordance with the commercial licence agreement provided with the       *
// Software or, alternatively, in accordance with the terms contained in a  *
// written agreement between you and Universidade do Porto. For licensing   *
// terms, conditions, and further information contact lsts@fe.up.pt.        *
//                                                                          *
// European Union Public Licence - EUPL v.1.1 Usage                         *
// Alternatively, this file may be used under the terms of the EUPL,        *
// Version 1.1 only (the "Licence"), appearing in the file LICENCE.md       *
// included in the packaging of this file. You may not use this work        *
// except in compliance with the Licence. Unless required by applicable     *
// law or agreed to in writing, software distributed under the Licence is   *
// distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF     *
// ANY KIND, either express or implied. See the Licence for the specific    *
// language governing permissions and limitations at                        *
// https://www.lsts.pt/dune/licence.                                        *
//***************************************************************************
// Author: Renato Caldas                                                    *
//***************************************************************************

// ISO C++ 98 headers.
#include <stdexcept>

// DUNE headers.
#include <DUNE/Tasks/Task.hpp>
#include <DUNE/Status/Messages.hpp>

namespace DUNE
{
  namespace Tasks
  {
    void
    Entity::setState(IMC::EntityState::StateEnum state,
                           Status::Code code)
    {
      bool new_state = (state != m_entity_state.state);

      m_entity_state.state = state;
      if (code != m_entity_state_code)
        m_entity_state.description = DTR(Status::getString(code));
      m_entity_state_code = code;

      if (new_state && (m_id != DUNE_IMC_CONST_UNK_EID))
        m_owner->dispatch(m_entity_state);
    }

    void
    Entity::setState(IMC::EntityState::StateEnum state,
                           const std::string& message)
    {
      bool new_state = (state != m_entity_state.state);

      m_entity_state.state = state;
      m_entity_state.description = message;
      m_entity_state_code = -1;

      if (new_state && (m_id != DUNE_IMC_CONST_UNK_EID))
        m_owner->dispatch(m_entity_state);
    }

    void
    Entity::reportState(void)
    {
      m_owner->dispatch(m_entity_state);
    }

    void
    Entity::reportActivationState(void)
    {
      m_owner->dispatch(m_act_state);
    }

    void
    Entity::requestActivation(void)
    {
      if (m_act_state.state != IMC::EntityActivationState::EAS_INACTIVE)
      {
        m_owner->spew("task is not inactive");

        if ((m_act_state.state == IMC::EntityActivationState::EAS_DEACT_IP)
            || (m_act_state.state == IMC::EntityActivationState::EAS_DEACT_DONE)
            || (m_act_state.state == IMC::EntityActivationState::EAS_DEACT_FAIL)
            || (m_act_state.state == IMC::EntityActivationState::EAS_ACT_FAIL))
        {
          m_owner->spew("saving activation request");
          m_next_act_state = NAS_ACTIVE;
        }
        else if (m_act_state.state == IMC::EntityActivationState::EAS_ACT_IP)
        {
          m_owner->spew("activation is in progress");
          m_next_act_state = NAS_ACTIVE;
        }

        m_owner->dispatch(m_act_state);
        return;
      }

      m_next_act_state = NAS_SAME;
      m_act_state.state = IMC::EntityActivationState::EAS_ACT_IP;
      m_owner->dispatch(m_act_state);
    }

    void
    Entity::requestDeactivation(void)
    {
      if (m_act_state.state != IMC::EntityActivationState::EAS_ACTIVE)
      {
        m_owner->spew("task is not active");

        if ((m_act_state.state == IMC::EntityActivationState::EAS_DEACT_FAIL)
            || (m_act_state.state == IMC::EntityActivationState::EAS_ACT_IP)
            || (m_act_state.state == IMC::EntityActivationState::EAS_ACT_DONE)
            || (m_act_state.state == IMC::EntityActivationState::EAS_ACT_FAIL))
        {
          m_owner->spew("saving deactivation request");
          m_next_act_state = NAS_INACTIVE;
        }
        else if (m_act_state.state == IMC::EntityActivationState::EAS_DEACT_IP)
        {
          m_owner->spew("deactivation is in progress");
          m_next_act_state = NAS_INACTIVE;
        }

        m_owner->dispatch(m_act_state);
        return;
      }

      m_next_act_state = NAS_SAME;
      m_act_state.state = IMC::EntityActivationState::EAS_DEACT_IP;
      m_owner->dispatch(m_act_state);
    }

    void
    Entity::failActivation(const std::string& reason)
    {
      m_act_state.state = IMC::EntityActivationState::EAS_ACT_FAIL;
      m_act_state.error = reason;
      m_owner->dispatch(m_act_state);

      m_act_state.state = IMC::EntityActivationState::EAS_INACTIVE;
      m_act_state.error.clear();
      m_owner->dispatch(m_act_state);
    }

    void
    Entity::succeedActivation(void)
    {
      m_act_state.state = IMC::EntityActivationState::EAS_ACT_DONE;
      m_owner->dispatch(m_act_state);

      m_act_state.state = IMC::EntityActivationState::EAS_ACTIVE;
      m_owner->dispatch(m_act_state);

      if (m_next_act_state == NAS_INACTIVE)
        requestDeactivation();
    }

    void
    Entity::succeedDeactivation(void)
    {
      m_act_state.state = IMC::EntityActivationState::EAS_DEACT_DONE;
      m_owner->dispatch(m_act_state);

      m_act_state.state = IMC::EntityActivationState::EAS_INACTIVE;
      m_owner->dispatch(m_act_state);

      if (m_next_act_state == NAS_ACTIVE)
        requestActivation();
    }

    void
    Entity::failDeactivation(const std::string& reason)
    {
      m_act_state.state = IMC::EntityActivationState::EAS_DEACT_FAIL;
      m_act_state.error = reason;
      m_owner->dispatch(m_act_state);

      m_act_state.state = IMC::EntityActivationState::EAS_ACTIVE;
      m_act_state.error.clear();
      m_owner->dispatch(m_act_state);
    }
  }
}
