/*
* ***** BEGIN GPL LICENSE BLOCK *****
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software Foundation,
* Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
*
*
* The Original Code is: all of this file.
*
* Contributor(s): none yet.
*
* ***** END GPL LICENSE BLOCK *****
*/

/** \file gameengine/GameLogic/SCA_VibrationActuator.cpp
*  \ingroup GameLogic
*/


#include "SCA_VibrationActuator.h"
#include "SCA_JoystickManager.h"
#include "PIL_time.h" // Module to get real time in Game Engine

SCA_VibrationActuator::SCA_VibrationActuator(SCA_IObject *gameobj, short mode, int joyindex, float strength_left,  float strength_right, int duration)
	: SCA_IActuator(gameobj, KX_ACT_VIBRATION),
	m_joyindex(joyindex),
    m_mode(mode),
	m_strength_left(strength_left),
    m_strength_right(strength_right),
	m_duration(duration),
	m_endtime(0.0f)
{
}

SCA_VibrationActuator::~SCA_VibrationActuator(void)
{
}

CValue* SCA_VibrationActuator::GetReplica(void)
{
	SCA_VibrationActuator *replica = new SCA_VibrationActuator(*this);
	replica->ProcessReplica();
	return replica;
}

bool SCA_VibrationActuator::Update()
{
	/* Can't init instance in constructor because m_joystick list is not available yet */
	SCA_JoystickManager *mgr = (SCA_JoystickManager *)GetLogicManager();
	DEV_Joystick *instance = mgr->GetJoystickDevice(m_joyindex);

	if (!instance) {
		return false;
	}

	if (IsPositiveEvent()) {
		switch (m_mode) {
			case KX_ACT_VIBRATION_PLAY:
			{
				instance->RumblePlay(m_strength_left, m_strength_right, m_duration);
				m_endtime = PIL_check_seconds_timer() * 1000.0f + m_duration;
				break;
			}
		case KX_ACT_VIBRATION_STOP:
			{
				instance->RumbleStop();
				m_endtime = 0.0f;
				break;
			}
		}
	}

	RemoveAllEvents();

	if (!(PIL_check_seconds_timer() * 1000.0f < m_endtime)) {
		instance->RumbleStop();
		m_endtime = 0.0f;
	}

	return instance->GetRumbleStatus();
}

#ifdef WITH_PYTHON

/* ------------------------------------------------------------------------- */
/* Python functions                                                          */
/* ------------------------------------------------------------------------- */



/* Integration hooks ------------------------------------------------------- */
PyTypeObject SCA_VibrationActuator::Type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"SCA_VibrationActuator",
	sizeof(PyObjectPlus_Proxy),
	0,
	py_base_dealloc,
	0,
	0,
	0,
	0,
	py_base_repr,
	0, 0, 0, 0, 0, 0, 0, 0, 0,
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
	0, 0, 0, 0, 0, 0, 0,
	Methods,
	0,
	0,
	&SCA_IActuator::Type,
	0, 0, 0, 0, 0, 0,
	py_base_new
};

PyMethodDef SCA_VibrationActuator::Methods[] = {
	KX_PYMETHODTABLE_NOARGS(SCA_VibrationActuator, startVibration),
	KX_PYMETHODTABLE_NOARGS(SCA_VibrationActuator, stopVibration),
	{ NULL, NULL } //Sentinel
};

PyAttributeDef SCA_VibrationActuator::Attributes[] = {
	KX_PYATTRIBUTE_INT_RW("duration", 0, INT_MAX, true, SCA_VibrationActuator, m_duration),
	KX_PYATTRIBUTE_INT_RW("joyindex", 0, 7, true, SCA_VibrationActuator, m_joyindex),
	KX_PYATTRIBUTE_FLOAT_RW("strengthLeft", 0.0, 1.0, SCA_VibrationActuator, m_strength_left),
	KX_PYATTRIBUTE_FLOAT_RW("strengthRight", 0.0, 1.0, SCA_VibrationActuator, m_strength_right),
	KX_PYATTRIBUTE_RO_FUNCTION("isVibrating", SCA_VibrationActuator, pyattr_get_statusVibration),
	KX_PYATTRIBUTE_RO_FUNCTION("hasVibration", SCA_VibrationActuator, pyattr_get_hasVibration),
	{ NULL }	//Sentinel
};

/* Methods ----------------------------------------------------------------- */
KX_PYMETHODDEF_DOC_NOARGS(SCA_VibrationActuator, startVibration,
"startVibration()\n"
"\tStarts the joystick vibration.\n")
{
	SCA_JoystickManager *mgr = (SCA_JoystickManager *)GetLogicManager();
	DEV_Joystick *instance = mgr->GetJoystickDevice(m_joyindex);

	if (!instance) {
		Py_RETURN_NONE;
	}

	instance->RumblePlay(m_strength_left, m_strength_right, m_duration);
	m_endtime = PIL_check_seconds_timer() * 1000.0f + m_duration;

	Py_RETURN_NONE;
}

KX_PYMETHODDEF_DOC_NOARGS(SCA_VibrationActuator, stopVibration,
"StopVibration()\n"
"\tStops the joystick vibration.\n")
{
	SCA_JoystickManager *mgr = (SCA_JoystickManager *)GetLogicManager();
	DEV_Joystick *instance = mgr->GetJoystickDevice(m_joyindex);

	if (!instance) {
		Py_RETURN_NONE;
	}

	instance->RumbleStop();
	m_endtime = 0.0f;

	Py_RETURN_NONE;
}


/* Attribute getting -------------------------------------------- */
PyObject *SCA_VibrationActuator::pyattr_get_statusVibration(void *self_v, const struct KX_PYATTRIBUTE_DEF *attrdef)
{
	SCA_VibrationActuator *self = static_cast<SCA_VibrationActuator *>(self_v);
	SCA_JoystickManager *mgr = (SCA_JoystickManager *)self->GetLogicManager();
	DEV_Joystick *instance = mgr->GetJoystickDevice(self->m_joyindex);

	if (!instance) {
		return Py_False;
	}

	return PyBool_FromLong(instance->GetRumbleStatus());
}

/* Attribute getting -------------------------------------------- */
PyObject *SCA_VibrationActuator::pyattr_get_hasVibration(void *self_v, const struct KX_PYATTRIBUTE_DEF *attrdef)
{
	SCA_VibrationActuator *self = static_cast<SCA_VibrationActuator *>(self_v);
	SCA_JoystickManager *mgr = (SCA_JoystickManager *)self->GetLogicManager();
	DEV_Joystick *instance = mgr->GetJoystickDevice(self->m_joyindex);

	if (!instance) {
		return Py_False;
	}

	return PyBool_FromLong(instance->GetRumbleSupport());
}

#endif // WITH_PYTHON
