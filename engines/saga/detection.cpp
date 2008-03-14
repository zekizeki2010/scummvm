/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $URL$
 * $Id$
 *
 */

// Game detection, general game parameters

#include "saga/saga.h"

#include "base/plugins.h"

#include "common/config-manager.h"
#include "common/advancedDetector.h"

#include "saga/displayinfo.h"
#include "saga/rscfile.h"
#include "saga/interface.h"
#include "saga/scene.h"
#include "saga/sagaresnames.h"

namespace Saga {
struct SAGAGameDescription {
	Common::ADGameDescription desc;

	int gameType;
	int gameId;
	uint32 features;
	int startSceneNumber;
	const GameResourceDescription *resourceDescription;
	int fontsCount;
	const GameFontDescription *fontDescriptions;
	const GameSoundInfo *voiceInfo;
	const GameSoundInfo *sfxInfo;
	int patchesCount;
	const GamePatchDescription *patchDescriptions;
};

const bool SagaEngine::isBigEndian() const { return (_gameDescription->features & GF_BIG_ENDIAN_DATA) != 0; }
const bool SagaEngine::isMacResources() const { return (getPlatform() == Common::kPlatformMacintosh); }
const GameResourceDescription *SagaEngine::getResourceDescription() { return _gameDescription->resourceDescription; }
const GameSoundInfo *SagaEngine::getVoiceInfo() const { return _gameDescription->voiceInfo; }
const GameSoundInfo *SagaEngine::getSfxInfo() const { return _gameDescription->sfxInfo; }
const GameSoundInfo *SagaEngine::getMusicInfo() const {
	static GameSoundInfo musicInfo;
	musicInfo.resourceType = kSoundPCM;
	musicInfo.frequency = 11025;
	musicInfo.sampleBits = 16;
	// The digital music in the ITE Mac demo version is not stereo
	musicInfo.stereo = _gameDescription->gameType == GID_ITE_MACDEMO2 ? false : true;
	musicInfo.isBigEndian = false;
	musicInfo.isSigned = true;

	return &musicInfo;
}

const GameFontDescription *SagaEngine::getFontDescription(int index) {
	assert(index < _gameDescription->fontsCount);
	return &_gameDescription->fontDescriptions[index];
}
int SagaEngine::getFontsCount() const { return _gameDescription->fontsCount; }

int SagaEngine::getGameId() const { return _gameDescription->gameId; }
int SagaEngine::getGameType() const { return _gameDescription->gameType; }

uint32 SagaEngine::getFeatures() const {
	uint32 result = _gameDescription->features;

	if (_gf_wyrmkeep)
		result |= GF_WYRMKEEP;

	if (_gf_compressed_sounds)
		result |= GF_COMPRESSED_SOUNDS;

	return result;
}

Common::Language SagaEngine::getLanguage() const { return _gameDescription->desc.language; }
Common::Platform SagaEngine::getPlatform() const { return _gameDescription->desc.platform; }
int SagaEngine::getGameNumber() const { return _gameNumber; }
int SagaEngine::getStartSceneNumber() const { return _gameDescription->startSceneNumber; }

int SagaEngine::getPatchesCount() const { return _gameDescription->patchesCount; }
const GamePatchDescription *SagaEngine::getPatchDescriptions() const { return _gameDescription->patchDescriptions; }
const Common::ADGameFileDescription *SagaEngine::getFilesDescriptions() const { return _gameDescription->desc.filesDescriptions; }

}

static const PlainGameDescriptor sagaGames[] = {
	{"saga", "SAGA Engine game"},
	{"ite", "Inherit the Earth: Quest for the Orb"},
	{"ihnm", "I Have No Mouth and I Must Scream"},
	{0, 0}
};

static const Common::ADObsoleteGameID obsoleteGameIDsTable[] = {
	{"ite", "saga", Common::kPlatformUnknown},
	{"ihnm", "saga", Common::kPlatformUnknown},
	{0, 0, Common::kPlatformUnknown}
};

#include "saga/detection_tables.h"

static const Common::ADParams detectionParams = {
	// Pointer to ADGameDescription or its superset structure
	(const byte *)Saga::gameDescriptions,
	// Size of that superset structure
	sizeof(Saga::SAGAGameDescription),
	// Number of bytes to compute MD5 sum for
	5000,
	// List of all engine targets
	sagaGames,
	// Structure for autoupgrading obsolete targets
	obsoleteGameIDsTable,
	// Name of single gameid (optional)
	"saga",
	// List of files for file-based fallback detection (optional)
	0,
	// Flags
	Common::kADFlagAugmentPreferredTarget
};

class SagaMetaEngine : public Common::AdvancedMetaEngine {
public:
	SagaMetaEngine() : Common::AdvancedMetaEngine(detectionParams) {}

	virtual const char *getName() const {
		return "Saga engine";
	}

	virtual const char *getCopyright() const {
		return "Inherit the Earth (C) Wyrmkeep Entertainment";
	}

	virtual bool createInstance(OSystem *syst, Engine **engine, const Common::EncapsulatedADGameDesc &encapsulatedDesc) const;
};

bool SagaMetaEngine::createInstance(OSystem *syst, Engine **engine, const Common::EncapsulatedADGameDesc &encapsulatedDesc) const {
	const Saga::SAGAGameDescription *gd = (const Saga::SAGAGameDescription *)(encapsulatedDesc.realDesc);
	if (gd) {
		*engine = new Saga::SagaEngine(syst, gd);
	}
	return gd != 0;
}

REGISTER_PLUGIN(SAGA, PLUGIN_TYPE_ENGINE, SagaMetaEngine);

namespace Saga {

bool SagaEngine::initGame() {
	_displayClip.right = getDisplayInfo().logicalWidth;
	_displayClip.bottom = getDisplayInfo().logicalHeight;

	return _resource->createContexts();
}

const GameDisplayInfo &SagaEngine::getDisplayInfo() {
	return _gameDescription->gameType == GType_ITE ? ITE_DisplayInfo : IHNM_DisplayInfo;
}

int SagaEngine::getDisplayWidth() const {
	const GameDisplayInfo &di = _gameDescription->gameType == GType_ITE ? ITE_DisplayInfo : IHNM_DisplayInfo;
	return di.logicalWidth;
}

int SagaEngine::getDisplayHeight() const {
	const GameDisplayInfo &di = _gameDescription->gameType == GType_ITE ? ITE_DisplayInfo : IHNM_DisplayInfo;
	return di.logicalHeight;
}

} // End of namespace Saga
