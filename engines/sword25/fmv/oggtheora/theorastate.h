// -----------------------------------------------------------------------------
// This file is part of Broken Sword 2.5
// Copyright (c) Malte Thiesen, Daniel Queteschiner and Michael Elsd�rfer
//
// Broken Sword 2.5 is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// Broken Sword 2.5 is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Broken Sword 2.5; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
// -----------------------------------------------------------------------------

#ifndef SWORD25_THEORASTATE_H
#define SWORD25_THEORASTATE_H

// -----------------------------------------------------------------------------
// Includes
// -----------------------------------------------------------------------------

#include "sword25/kernel/common.h"
#include <theora/theora.h>

// -----------------------------------------------------------------------------
// Klassendefinition
// -----------------------------------------------------------------------------

class BS_TheoraState
{
public:
	BS_TheoraState();
	virtual ~BS_TheoraState();

	int		DecodeHeader(ogg_packet * OggPacketPtr);
	int		DecodeInit();
	int		DecodePacketIn(ogg_packet * OggPacketPtr);
	int		DecodeYUVOut(yuv_buffer * YUV);
	double	GranuleTime();

	const theora_info & GetInfo() const { return m_Info; }

private:
	theora_info					m_Info;
	theora_comment				m_Comment;
	bool						m_StateInitialized;
	theora_state				m_State;
};

#endif