/* ScummVM - Scumm Interpreter
 * Copyright (C) 2002 The ScummVM project
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header$
 */

#include <ctype.h>

#include "stdafx.h"
#include "newgui.h"
#include "dialog.h"
#include "widget.h"
#include "scumm.h"
#include "ListWidget.h"
#include "config-file.h"
#include "sound/mididrv.h"
#include "sound/imuse.h"

Dialog::~Dialog()
{
	teardownScreenBuf();
}

void Dialog::setupScreenBuf()
{
	// Create _screenBuf if it doesn't already exist
	if (!_screenBuf)
		_screenBuf = new byte[320*200];
	
	// Draw the fixed parts of the dialog: background and border.
	_gui->blendRect(_x, _y, _w, _h, _gui->_bgcolor);
	_gui->box(_x, _y, _w, _h);
	
	// Finally blit this to _screenBuf
	_gui->blitTo(_screenBuf, _x, _y, _w, _h); 
}

void Dialog::teardownScreenBuf()
{
	if (_screenBuf) {
		delete [] _screenBuf;
		_screenBuf = 0;
	}
}

void Dialog::open()
{
	_gui->openDialog(this);
}

void Dialog::close()
{
	if (_mouseWidget) {
		_mouseWidget->handleMouseLeft(0);
		_mouseWidget = 0;
	}
	if (_focusedWidget) {
		_focusedWidget->lostFocus();
		_focusedWidget = 0;
	}
	_gui->closeTopDialog();
}

void Dialog::draw()
{
	Widget *w = _firstWidget;

	if (_screenBuf) {
		_gui->blitFrom(_screenBuf, _x, _y, _w, _h); 
	} else {
		_gui->fillRect(_x, _y, _w, _h, _gui->_bgcolor);
		_gui->box(_x, _y, _w, _h);
	}
	_gui->addDirtyRect(_x, _y, _w, _h);

	while (w) {
		w->draw();
		w = w->_next;
	}
}

void Dialog::handleMouseDown(int x, int y, int button)
{
	Widget *w;
	w = findWidget(x, y);

	if (w != _focusedWidget) {
		// The focus will change. Tell the old focused widget (if any)
		// that it lost the focus.
		if (_focusedWidget)
			_focusedWidget->lostFocus();
	
		// Tell the new focused widget (if any) that it just gained the focus.
		if (w)
			w->receivedFocus();
	
		_focusedWidget = w;
	}

	if (_focusedWidget)
		_focusedWidget->handleMouseDown(x - _focusedWidget->_x, y - _focusedWidget->_y, button);
}

void Dialog::handleMouseUp(int x, int y, int button)
{
	Widget *w;
	
	if (_focusedWidget) {
		w = _focusedWidget;
		
		// Lose focus on mouseup unless the widget requested to retain the focus
		if (! (_focusedWidget->getFlags() & WIDGET_RETAIN_FOCUS )) {
			_focusedWidget->lostFocus();
			_focusedWidget = 0;
		}

	} else {
		w = findWidget(x, y);
	}

	if (w)
		w->handleMouseUp(x - w->_x, y - w->_y, button);
}

void Dialog::handleKeyDown(char key, int modifiers)
{
	// ESC closes all dialogs by default
	if (key == 27)
		close();
	
	if (_focusedWidget) {
		_focusedWidget->handleKeyDown(key, modifiers);
	} else {
		// Hotkey handling
		Widget *w = _firstWidget;
		key = toupper(key);
		while (w) {
			if (w->_type == kButtonWidget && key == toupper(((ButtonWidget *)w)->_hotkey)) {
				// We first send a mouseDown then a mouseUp.
				// FIXME: insert a brief delay between both.
				w->handleMouseDown(0, 0, 1);
				w->handleMouseUp(0, 0, 1);
				break;
			}
			w = w->_next;
		}
	}
}

void Dialog::handleKeyUp(char key, int modifiers)
{
	// Focused widget recieves keyup events
	if (_focusedWidget)
		_focusedWidget->handleKeyUp(key, modifiers);
}

void Dialog::handleMouseMoved(int x, int y, int button)
{
	Widget *w;
	
	if (_focusedWidget) {
		w = _focusedWidget;
		
		// We still send mouseEntered/Left messages to the focused item
		// (but to no other items).
		bool mouseInFocusedWidget = (x >= w->_x && x < w->_x + w->_w && y >= w->_y && y < w->_y + w->_h);
		if (mouseInFocusedWidget && _mouseWidget != w) {
			_mouseWidget = w;
			w->handleMouseEntered(button);
		} else if (!mouseInFocusedWidget && _mouseWidget == w) {
			_mouseWidget = 0;
			w->handleMouseLeft(button);
		}
			
	} else {
		w = findWidget(x, y);

		if (_mouseWidget != w) {
			if (_mouseWidget)
				_mouseWidget->handleMouseLeft(button);
			if (w)
				w->handleMouseEntered(button);
			_mouseWidget = w;
		} 

		if (!w || !(w->getFlags() & WIDGET_TRACK_MOUSE)) {
			return;
		}
	}
	
	w->handleMouseMoved(x - w->_x, y - w->_y, button);
}

void Dialog::handleTickle()
{
	// Focused widget recieves tickle notifications
	if (_focusedWidget && _focusedWidget->getFlags() & WIDGET_WANT_TICKLE) {
		_focusedWidget->handleTickle();
	}
}

void Dialog::handleCommand(CommandSender *sender, uint32 cmd, uint32 data)
{
	switch (cmd) {
	case kCloseCmd:
		close();
		break;
	}
}

/*
 * Determine the widget at location (x,y) if any. Assumes the coordinates are
 * in the local coordinate system, i.e. relative to the top left of the dialog.
 */
Widget *Dialog::findWidget(int x, int y)
{
	Widget *w = _firstWidget;
	while (w) {
		// Stop as soon as we find a widget that contains the point (x,y)
		if (x >= w->_x && x < w->_x + w->_w && y >= w->_y && y < w->_y + w->_h)
			break;
		w = w->_next;
	}
	return w;
}

void Dialog::addResText(int x, int y, int w, int h, int resID)
{
	// Get the string
	const char *str = _gui->queryResString(resID);
	if (!str)
		str = "Dummy!";
	new StaticTextWidget(this, x, y, w, h, str, kTextAlignLeft);
}

void Dialog::addButton(int x, int y, int w, int h, const char *label, uint32 cmd, char hotkey)
{
	new ButtonWidget(this, x, y, w, h, label, cmd, hotkey);
}

#pragma mark -


enum {
	kSaveCmd = 'SAVE',
	kLoadCmd = 'LOAD',
	kPlayCmd = 'PLAY',
	kOptionsCmd = 'OPTN',
	kQuitCmd = 'QUIT'
};

/*
 * TODO
 * - Maybe go back to the old way of differentiating between the save and the load mode?
 *   This would include that in the load mode the list is not editable.
 * - Currently the savegame list is only loaded once when the dialog is created. Instead,
 *   it should be loaded whenever the dialog is opened. Might want to add an open()
 *   method to Dialog for that.
 */

SaveLoadDialog::SaveLoadDialog(NewGui *gui)
	: Dialog (gui, 30, 20, 260, 124)
{
	addResText(10, 7, 240, 16, 1);
//  addResText(10, 7, 240, 16, 2);
//  addResText(10, 7, 240, 16, 3);

	addButton(200, 20, 54, 16, RES_STRING(4), kSaveCmd, 'S');	// Save
	addButton(200, 40, 54, 16, RES_STRING(5), kLoadCmd, 'L');	// Load
	addButton(200, 60, 54, 16, RES_STRING(6), kPlayCmd, 'P');	// Play
	addButton(200, 80, 54, 16, CUSTOM_STRING(17), kOptionsCmd, 'O');	// Options
	addButton(200, 100, 54, 16, RES_STRING(8), kQuitCmd, 'Q');	// Quit
	
	// FIXME - test
	new CheckboxWidget(this, 10, 20, 90, 16, "Toggle me", 0);
	
	// FIXME - test
	_savegameList = new ListWidget(this, 10, 40, 180, 74);

	// Get savegame names
	ScummVM::StringList l;
	char name[32];
	Scumm *s = _gui->getScumm();

	for (int i = 0; i <= 80; i++) {		// 80 I got from old gui
		s->getSavegameName(i, name);
		l.push_back(name);
	}

	_savegameList->setList(l);
}

void SaveLoadDialog::handleCommand(CommandSender *sender, uint32 cmd, uint32 data)
{
	switch (cmd) {
	case kSaveCmd:
		if (_savegameList->getSelectedString()[0] != 0) {
			Scumm *s = _gui->getScumm();
			s->_saveLoadSlot = _savegameList->getSelected();
			s->_saveLoadCompatible = false;
			s->_saveLoadFlag = 1;		// 1 for save, I assume (Painelf)
			strcpy(s->_saveLoadName, _savegameList->getSelectedString());
			close();
		}
		break;
	case kLoadCmd:
		if (_savegameList->getSelectedString()[0] != 0) {
			Scumm *s = _gui->getScumm();
			s->_saveLoadSlot = _savegameList->getSelected();
			s->_saveLoadCompatible = false;
			s->_saveLoadFlag = 2;		// 2 for load. Magic number anyone?
			close();
		}
		break;
	case kPlayCmd:
		close();
		break;
	case kOptionsCmd:
		_gui->optionsDialog();
		break;
	case kQuitCmd:
		exit(1);
		break;
	default:
		Dialog::handleCommand(sender, cmd, data);
	}
}


#pragma mark -

enum {
	kSoundCmd = 'SOUN',
	kKeysCmd = 'KEYS',
	kAboutCmd = 'ABOU',
	kMiscCmd = 'OPTN'
};

OptionsDialog::OptionsDialog(NewGui *gui)
	: Dialog (gui, 50, 80, 210, 60)
{
	addButton( 10, 10, 40, 16, CUSTOM_STRING(5), kSoundCmd, 'S');	// Sound
	addButton( 80, 10, 40, 16, CUSTOM_STRING(6), kKeysCmd, 'K');	// Keys
	addButton(150, 10, 40, 16, CUSTOM_STRING(7), kAboutCmd, 'A');	// About
	addButton( 10, 35, 40, 16, CUSTOM_STRING(18), kMiscCmd, 'M');	// Misc
	addButton(150, 35, 40, 16, CUSTOM_STRING(23), kCloseCmd, 'C');	// Close dialog - FIXME
}

void OptionsDialog::handleCommand(CommandSender *sender, uint32 cmd, uint32 data)
{
	switch (cmd) {
	case kSoundCmd:
		_gui->soundDialog();
		break;
	case kKeysCmd:
		break;
	case kAboutCmd:
		_gui->aboutDialog();
		break;
	case kMiscCmd:
		break;
	default:
		Dialog::handleCommand(sender, cmd, data);
	}
}


#pragma mark -

AboutDialog::AboutDialog(NewGui *gui)
	: Dialog (gui, 30, 10, 260, 134)
{
	addButton(110, 110, 40, 16, CUSTOM_STRING(23), kCloseCmd, 'C');	// Close dialog - FIXME
	new StaticTextWidget(this, 10, 17, 240, 16, "Build " SCUMMVM_VERSION " (" SCUMMVM_CVS ")", kTextAlignCenter);
	new StaticTextWidget(this, 10, 37, 240, 16, "ScummVM http://scummvm.sourceforge.net", kTextAlignCenter);
	new StaticTextWidget(this, 10, 67, 240, 16, "All games (c) LucasArts", kTextAlignCenter);
	new StaticTextWidget(this, 10, 84, 240, 16, "Except", kTextAlignCenter);
	new StaticTextWidget(this, 10, 97, 240, 16, "Simon the Sorcerer (c) Adventuresoft", kTextAlignCenter);
}

PauseDialog::PauseDialog(NewGui *gui)
	: Dialog (gui, 50, 80, 220, 16)
{
	addResText(4, 4, 220, 16, 10);
}

SoundDialog::SoundDialog(NewGui *gui)
	: Dialog (gui, 30, 20, 260, 110)
{

	// set up dialog
	addButton(70, 90, 54, 16, "OK", kOKCmd, 'O');	// Confirm dialog
	addButton(136, 90, 54, 16, "Cancel", kCancelCmd, 'C');	// Abort dialog
	new StaticTextWidget(this, 20, 17, 85, 16, "Master volume:", kTextAlignRight);
	new StaticTextWidget(this, 20, 37, 85, 16, "Music volume:", kTextAlignRight);
	new StaticTextWidget(this, 20, 57, 85, 16, "SFX volume:", kTextAlignRight);

	masterVolumeSlider = new SliderWidget(this, 110, 13, 80, 16, "Volume1", kMasterVolumeChanged);
	musicVolumeSlider = new SliderWidget(this, 110, 33, 80, 16, "Volume2", kMusicVolumeChanged);
	sfxVolumeSlider = new SliderWidget(this, 110, 53, 80, 16, "Volume3", kSfxVolumeChanged);

	masterVolumeSlider->setMinValue(0);	masterVolumeSlider->setMaxValue(255);
	musicVolumeSlider->setMinValue(0);	musicVolumeSlider->setMaxValue(255);
	sfxVolumeSlider->setMinValue(0);	sfxVolumeSlider->setMaxValue(255);

	masterVolumeLabel = new StaticTextWidget(this, 195, 17, 60, 16, "Volume1", kTextAlignLeft);
	musicVolumeLabel = new StaticTextWidget(this, 195, 37, 60, 16, "Volume2", kTextAlignLeft);
	sfxVolumeLabel = new StaticTextWidget(this, 195, 57, 60, 16, "Volume3", kTextAlignLeft);
	
	masterVolumeLabel->setFlags(WIDGET_CLEARBG);
	musicVolumeLabel->setFlags(WIDGET_CLEARBG);
	sfxVolumeLabel->setFlags(WIDGET_CLEARBG);
}

void SoundDialog::open()
{
	Scumm	*scumm = _gui->getScumm();
	
	Dialog::open();

	// get current variables
	_soundVolumeMaster = scumm->_sound_volume_master;
	_soundVolumeMusic = scumm->_sound_volume_music;
	_soundVolumeSfx = scumm->_sound_volume_sfx;

	masterVolumeSlider->setValue(_soundVolumeMaster);
	musicVolumeSlider->setValue(_soundVolumeMusic);
	sfxVolumeSlider->setValue(_soundVolumeSfx);

	masterVolumeLabel->setValue(_soundVolumeMaster);
	musicVolumeLabel->setValue(_soundVolumeMusic);
	sfxVolumeLabel->setValue(_soundVolumeSfx);
}


void SoundDialog::handleCommand(CommandSender *sender, uint32 cmd, uint32 data)
{
	switch (cmd) {
	case kMasterVolumeChanged:
		_soundVolumeMaster = masterVolumeSlider->getValue();
		masterVolumeLabel->setValue(_soundVolumeMaster);
		masterVolumeLabel->draw();
		break;
	case kMusicVolumeChanged:
		_soundVolumeMusic = musicVolumeSlider->getValue();
		musicVolumeLabel->setValue(_soundVolumeMusic);
		musicVolumeLabel->draw();
		break;
	case kSfxVolumeChanged:
		_soundVolumeSfx = sfxVolumeSlider->getValue();
		sfxVolumeLabel->setValue(_soundVolumeSfx);
		sfxVolumeLabel->draw();
		break;
	case kOKCmd: {
		Scumm	*scumm = _gui->getScumm();
		
		// FIXME: Look at Fingolfins comments in Gui::handleSoundDialogCommand(), gui.cpp 
		scumm->_sound_volume_master = _soundVolumeMaster;	// Master
		scumm->_sound_volume_music = _soundVolumeMusic;	// Music
		scumm->_sound_volume_sfx = _soundVolumeSfx;	// SFX
		
		scumm->_imuse->set_music_volume(_soundVolumeMusic);
		scumm->_imuse->set_master_volume(_soundVolumeMaster);
		scumm->_mixer->set_volume(_soundVolumeSfx);
		scumm->_mixer->set_music_volume(_soundVolumeMusic);
		
		scummcfg->set("master_volume", _soundVolumeMaster);
		scummcfg->set("music_volume", _soundVolumeMusic);
		scummcfg->set("sfx_volume", _soundVolumeSfx);
		scummcfg->flush();
		}
	case kCancelCmd:
		close();
		break;
	default:
		Dialog::handleCommand(sender, cmd, data);
	}
}
