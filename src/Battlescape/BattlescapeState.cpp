/*
 * Copyright 2010-2014 OpenXcom Developers.
 *
 * This file is part of OpenXcom.
 *
 * OpenXcom is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * OpenXcom is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom.  If not, see <http://www.gnu.org/licenses/>.
 */
#define _USE_MATH_DEFINES
#include <cmath>
#include <sstream>
#include <iomanip>
#include <SDL_gfxPrimitives.h>
#include "../lodepng.h"
#include "Map.h"
#include "Camera.h"
#include "BattlescapeState.h"
#include "NextTurnState.h"
#include "AbortMissionState.h"
#include "BattleState.h"
#include "UnitTurnBState.h"
#include "UnitWalkBState.h"
#include "ProjectileFlyBState.h"
#include "ExplosionBState.h"
#include "TileEngine.h"
#include "ActionMenuState.h"
#include "UnitInfoState.h"
#include "UnitDieBState.h"
#include "InventoryState.h"
#include "AlienBAIState.h"
#include "CivilianBAIState.h"
#include "Pathfinding.h"
#include "BattlescapeGame.h"
#include "../Engine/Game.h"
#include "../Engine/Options.h"
#include "../Engine/Music.h"
#include "../Engine/Language.h"
#include "../Engine/Palette.h"
#include "../Engine/Surface.h"
#include "../Engine/SurfaceSet.h"
#include "../Engine/Screen.h"
#include "../Engine/Sound.h"
#include "../Engine/Action.h"
#include "../Resource/ResourcePack.h"
#include "../Interface/Cursor.h"
#include "../Interface/FpsCounter.h"
#include "../Interface/Text.h"
#include "../Interface/Bar.h"
#include "../Interface/ImageButton.h"
#include "../Interface/NumberText.h"
#include "../Savegame/SavedGame.h"
#include "../Savegame/SavedBattleGame.h"
#include "../Savegame/Tile.h"
#include "../Savegame/BattleUnit.h"
#include "../Savegame/Soldier.h"
#include "../Savegame/BattleItem.h"
#include "../Ruleset/Ruleset.h"
#include "../Ruleset/RuleItem.h"
#include "../Ruleset/AlienDeployment.h"
#include "../Ruleset/Armor.h"
#include "../Engine/Timer.h"
#include "WarningMessage.h"
#include "../Menu/PauseState.h"
#include "DebriefingState.h"
#include "../Engine/RNG.h"
#include "InfoboxState.h"
#include "MiniMapState.h"
#include "BattlescapeGenerator.h"
#include "BriefingState.h"
#include "../Geoscape/DefeatState.h"
#include "../Geoscape/VictoryState.h"
#include "../Engine/Logger.h"
#include "../Engine/CrossPlatform.h"
#include "../Menu/LoadGameState.h"
#include "../Menu/SaveGameState.h"
#include "../fmath.h"
#include "../Engine/UIBuilder.h"

namespace OpenXcom
{

/**
 * Initializes all the elements in the Battlescape screen.
 * @param game Pointer to the core game.
 */
BattlescapeState::BattlescapeState() : _reserve(0), _popups(), _xBeforeMouseScrolling(0), _yBeforeMouseScrolling(0), _totalMouseMoveX(0), _totalMouseMoveY(0), _mouseMovedOverThreshold(0), _swipeFromSoldier(false), _multiGestureProcess(false)
{
	std::fill_n(_visibleUnit, 10, (BattleUnit*)(0));

	// Construct the action handlers map for UI builder
	std::map<std::string, ActionHandler> handlers;

	// Likewise, construct a map for hotkeys
	std::map<std::string, SDL_Keycode> hotkeys;

#define ADD_ACTION_HANDLER(x) handlers[#x] = (ActionHandler)&BattlescapeState::x
#define ADD_HOTKEY(x) hotkeys[#x] = Options::x

	ADD_ACTION_HANDLER(mouseInIcons);
	ADD_ACTION_HANDLER(mouseOutIcons);
	ADD_ACTION_HANDLER(btnUnitUpClick);
	ADD_ACTION_HANDLER(btnUnitDownClick);
	ADD_ACTION_HANDLER(btnMapUpClick);
	ADD_ACTION_HANDLER(btnMapDownClick);
	ADD_ACTION_HANDLER(btnShowMapClick);
	ADD_ACTION_HANDLER(btnKneelClick);
	ADD_ACTION_HANDLER(btnInventoryClick);
	ADD_ACTION_HANDLER(btnCenterClick);
	ADD_ACTION_HANDLER(btnNextSoldierClick);
	ADD_ACTION_HANDLER(btnPrevSoldierClick);
	ADD_ACTION_HANDLER(btnNextStopClick);
	ADD_ACTION_HANDLER(btnShowLayersClick);
	ADD_ACTION_HANDLER(btnHelpClick);
	ADD_ACTION_HANDLER(btnEndTurnClick);
	ADD_ACTION_HANDLER(btnAbortClick);
	ADD_ACTION_HANDLER(btnStatsClick);
	ADD_ACTION_HANDLER(btnLeftHandItemClick);
	ADD_ACTION_HANDLER(btnRightHandItemClick);
	ADD_ACTION_HANDLER(btnVisibleUnitClick);
	ADD_ACTION_HANDLER(btnLaunchClick);
	ADD_ACTION_HANDLER(btnPsiClick);
	ADD_ACTION_HANDLER(btnReserveClick);
	ADD_ACTION_HANDLER(btnReloadClick);
	ADD_ACTION_HANDLER(btnPersonalLightingClick);
	ADD_ACTION_HANDLER(btnReserveKneelClick);
	ADD_ACTION_HANDLER(btnZeroTUsClick);
	ADD_ACTION_HANDLER(txtTooltipIn);
	ADD_ACTION_HANDLER(txtTooltipOut);
	
	ADD_HOTKEY(keyBattleLevelUp);
	ADD_HOTKEY(keyBattleLevelDown);
	ADD_HOTKEY(keyBattleMap);
	ADD_HOTKEY(keyBattleKneel);
	ADD_HOTKEY(keyBattleInventory);
	ADD_HOTKEY(keyBattleCenterUnit);
	ADD_HOTKEY(keyBattleNextUnit);
	ADD_HOTKEY(keyBattlePrevUnit);
	ADD_HOTKEY(keyBattleDeselectUnit);
	ADD_HOTKEY(keyBattleOptions);
	ADD_HOTKEY(keyBattleEndTurn);
	ADD_HOTKEY(keyBattleAbort);
	ADD_HOTKEY(keyBattleStats);
	ADD_HOTKEY(keyBattleUseLeftHand);
	ADD_HOTKEY(keyBattleUseRightHand);
	ADD_HOTKEY(keyBattleReserveNone);
	ADD_HOTKEY(keyBattleReserveSnap);
	ADD_HOTKEY(keyBattleReserveAimed);
	ADD_HOTKEY(keyBattleReserveAuto);
	ADD_HOTKEY(keyBattleReserveKneel);
	ADD_HOTKEY(keyBattleZeroTUs);
	ADD_HOTKEY(keyBattleReload);
	ADD_HOTKEY(keyBattlePersonalLighting);

#undef ADD_ACTION_HANDLER
#undef ADD_HOTKEY

	// Some on-screen elements will use these variables.

	int screenWidth = Options::baseXResolution;
	int screenHeight = Options::baseYResolution;
	int iconsWidth = Map::ICON_WIDTH;
	int iconsHeight = Map::ICON_HEIGHT;
	_mouseOverIcons = false;

	// Create the battlemap view
	// the actual map height is the total height minus the height of the buttonbar
	int visibleMapHeight = screenHeight - iconsHeight;
	_map = new Map(_game, screenWidth, screenHeight, 0, 0, visibleMapHeight);
	// Set palette as early as possible - it's needed for UI creation.
	_game->getSavedGame()->getSavedBattle()->setPaletteByDepth(this);	
	// Since most of the UI elements will be added during the load,
	// we need to make sure map will be the first thing added.
	add(_map);

	// Load the UI elements. Note that not all UI elements are created from the layout file!
	_bUI = UIBuilder::buildUI(this, 
				_game->getResourcePack(),
				handlers,
				hotkeys, 
				CrossPlatform::getDataFile("Resources/UI/XCom1Battle.layout"));

	// Store pointers to those objects that we'll need.
	// Note that other entries in the _bUI map may be completely ignored - they're already add()'ed to the state.
	_icons = dynamic_cast<InteractiveSurface*>(_bUI["icons"]);
	_rank = _bUI["rank"];
	// Assign those pointers that we'll need, ignore others.
	// Since _bUI maps UI element names to the pointers to Surface class objects,
	// you'll need to explicitly cast the right class.
	_btnLeftHandItem = dynamic_cast<InteractiveSurface*>(_bUI["btnLeftHandItem"]);
	_btnRightHandItem = dynamic_cast<InteractiveSurface*>(_bUI["btnRightHandItem"]);

	_btnLaunch = dynamic_cast<InteractiveSurface*>(_bUI["btnLaunch"]);
	_btnPsi = dynamic_cast<InteractiveSurface*>(_bUI["btnPsi"]);

	showLaunchButton(false);
	showPsiButton(false);

	_btnReserveNone = dynamic_cast<ImageButton*>(_bUI["btnReserveNone"]);
	_btnReserveSnap = dynamic_cast<ImageButton*>(_bUI["btnReserveSnap"]);
	_btnReserveAimed = dynamic_cast<ImageButton*>(_bUI["btnReserveAimed"]);
	_btnReserveAuto = dynamic_cast<ImageButton*>(_bUI["btnReserveAuto"]);
	_btnReserveKneel = dynamic_cast<ImageButton*>(_bUI["btnReserveKneel"]);
	_btnZeroTUs = dynamic_cast<ImageButton*>(_bUI["btnZeroTUs"]);

	_numLayers = dynamic_cast<NumberText*>(_bUI["numLayers"]);
	_numAmmoLeft = dynamic_cast<NumberText*>(_bUI["numAmmoLeft"]);
	_numAmmoRight = dynamic_cast<NumberText*>(_bUI["numAmmoRight"]);
	_numTimeUnits = dynamic_cast<NumberText*>(_bUI["numTimeUnits"]);
	_numEnergy = dynamic_cast<NumberText*>(_bUI["numEnergy"]);
	_numHealth = dynamic_cast<NumberText*>(_bUI["numHealth"]);
	_numMorale = dynamic_cast<NumberText*>(_bUI["numMorale"]);

	_barTimeUnits = dynamic_cast<Bar*>(_bUI["barTimeUnits"]);
	_barEnergy = dynamic_cast<Bar*>(_bUI["barEnergy"]);
	_barHealth = dynamic_cast<Bar*>(_bUI["barHealth"]);
	_barMorale = dynamic_cast<Bar*>(_bUI["barMorale"]);

	_txtName = dynamic_cast<Text*>(_bUI["txtName"]);
	_txtDebug = dynamic_cast<Text*>(_bUI["txtDebug"]);
	_txtTooltip = dynamic_cast<Text*>(_bUI["txtTooltip"]);

#ifdef __ANDROID__
	_leftWpnActive = _bUI["leftWpnActive"];
	_rightWpnActive = _bUI["rightWpnActive"];
#endif
	// Buttons for visible units are still created here. That might or might not change in the future.
	for (int i = 0; i < VISIBLE_MAX; ++i)
	{
		_btnVisibleUnit[i] = new InteractiveSurface(15, 12, _icons->getX() + iconsWidth - 20, _icons->getY() - 16 - (i * 13));
		_numVisibleUnit[i] = new NumberText(15, 12, _icons->getX() + iconsWidth - 14 , _icons->getY() - 12 - (i * 13));
	}
	_numVisibleUnit[9]->setX(_numVisibleUnit[9]->getX() - 2); // center number 10
	_warning = new WarningMessage(224, 24, _icons->getX() + 48, _icons->getY() + 32);

	// Fix system colors
	_game->getCursor()->setColor(Palette::blockOffset(9));
	_game->getFpsCounter()->setColor(Palette::blockOffset(9));

	for (int i = 0; i < VISIBLE_MAX; ++i)
	{
		add(_btnVisibleUnit[i]);
		add(_numVisibleUnit[i]);
	}
	add(_warning);

	// Set up objects
	_save = _game->getSavedGame()->getSavedBattle();
	_map->init();
	_map->onMouseOver((ActionHandler)&BattlescapeState::mapOver);
	_map->onMousePress((ActionHandler)&BattlescapeState::mapPress);
	_map->onMouseRelease((ActionHandler)&BattlescapeState::mapRelease);
	_map->onMouseClick((ActionHandler)&BattlescapeState::mapClick, 0);
	_map->onMouseIn((ActionHandler)&BattlescapeState::mapIn);
	_map->onMultiGesture((ActionHandler)&BattlescapeState::multiGesture);
	_map->onFingerMotion((ActionHandler)&BattlescapeState::fingerMotion);

	SDL_Keycode buttons[] = {Options::keyBattleCenterEnemy1,
						Options::keyBattleCenterEnemy2,
						Options::keyBattleCenterEnemy3,
						Options::keyBattleCenterEnemy4,
						Options::keyBattleCenterEnemy5,
						Options::keyBattleCenterEnemy6,
						Options::keyBattleCenterEnemy7,
						Options::keyBattleCenterEnemy8,
						Options::keyBattleCenterEnemy9,
						Options::keyBattleCenterEnemy10};
	for (int i = 0; i < VISIBLE_MAX; ++i)
	{
		std::ostringstream tooltip;
		_btnVisibleUnit[i]->onMousePress((ActionHandler)&BattlescapeState::consumeEvent);
		_btnVisibleUnit[i]->onMouseRelease((ActionHandler)&BattlescapeState::consumeEvent);
		_btnVisibleUnit[i]->onMouseClick((ActionHandler)&BattlescapeState::btnVisibleUnitClick);
		_btnVisibleUnit[i]->onKeyboardPress((ActionHandler)&BattlescapeState::btnVisibleUnitClick, buttons[i]);
		tooltip << "STR_CENTER_ON_ENEMY_" << (i+1);
		_btnVisibleUnit[i]->setTooltip(tooltip.str());
		_btnVisibleUnit[i]->onMouseIn((ActionHandler)&BattlescapeState::txtTooltipIn);
		_btnVisibleUnit[i]->onMouseOut((ActionHandler)&BattlescapeState::txtTooltipOut);
		_numVisibleUnit[i]->setColor(16);
		_numVisibleUnit[i]->setValue(i+1);
	}
	_warning->setColor(Palette::blockOffset(2));
	_warning->setTextColor(Palette::blockOffset(1));

	_btnReserveNone->setGroup(&_reserve);
	_btnReserveSnap->setGroup(&_reserve);
	_btnReserveAimed->setGroup(&_reserve);
	_btnReserveAuto->setGroup(&_reserve);

	// Set music
	_game->getResourcePack()->playMusic("GMTACTIC");

	_animTimer = new Timer(DEFAULT_ANIM_SPEED, true);
	_animTimer->onTimer((StateHandler)&BattlescapeState::animate);

	_gameTimer = new Timer(DEFAULT_ANIM_SPEED, true);
	_gameTimer->onTimer((StateHandler)&BattlescapeState::handleState);
	
#ifdef __ANDROID__
	_longPressTimer = new Timer(500, false);
	_longPressTimer->onTimer((StateHandler)&BattlescapeState::mapLongPress);
#endif

	_battleGame = new BattlescapeGame(_save, this);

	_firstInit = true;
	_isMouseScrolling = false;
	_isMouseScrolled = false;
	_currentTooltip = "";
}


/**
 * Deletes the battlescapestate.
 */
BattlescapeState::~BattlescapeState()
{
#ifdef __ANDROID__
	delete _longPressTimer;
#endif
	delete _animTimer;
	delete _gameTimer;
	delete _battleGame;
}

/**
 * Initilizes the battlescapestate.
 */
void BattlescapeState::init()
{
	State::init();
	_animTimer->start();
	_gameTimer->start();
	_map->setFocus(true);
	_map->cacheUnits();
	_map->draw();
	_battleGame->init();
	updateSoldierInfo();
	// Update reserve settings
	_battleGame->setTUReserved(_save->getTUReserved(), true);
	switch (_save->getTUReserved())
	{
	case BA_SNAPSHOT:
		_reserve = _btnReserveSnap;
		break;
	case BA_AIMEDSHOT:
		_reserve = _btnReserveAimed;
		break;
	case BA_AUTOSHOT:
		_reserve = _btnReserveAuto;
		break;
	default:
		_reserve = _btnReserveNone;
		break;
	}
	if (_firstInit && playableUnitSelected())
	{
		_battleGame->setupCursor();
		_map->getCamera()->centerOnPosition(_save->getSelectedUnit()->getPosition());
		_firstInit = false;
		_btnReserveNone->setGroup(&_reserve);
		_btnReserveSnap->setGroup(&_reserve);
		_btnReserveAimed->setGroup(&_reserve);
		_btnReserveAuto->setGroup(&_reserve);
	}
	_txtTooltip->setText(L"");
	_btnReserveKneel->toggle(_save->getKneelReserved());
	_battleGame->setKneelReserved(_save->getKneelReserved());
}

/**
 * Runs the timers and handles popups.
 */
void BattlescapeState::think()
{
	static bool popped = false;

	if (_gameTimer->isRunning())
	{
		if (_popups.empty())
		{
			State::think();
			_battleGame->think();
			_animTimer->think(this, 0);
			_gameTimer->think(this, 0);
#ifdef __ANDROID__
			_longPressTimer->think(this, 0);
#endif
			if (popped)
			{
				_battleGame->handleNonTargetAction();
				popped = false;
			}
		}
		else
		{
			// Handle popups
			_game->pushState(*_popups.begin());
			_popups.erase(_popups.begin());
			popped = true;
			return;
		}
	}
}

/**
 * Processes any mouse moving over the map.
 * @param action Pointer to an action.
 */
void BattlescapeState::mapOver(Action *action)
{
#ifdef __ANDROID__
	/* Ignore dragging if we're just turning our unit */
	if (_swipeFromSoldier)
	{
		return;
	}
#endif
	if (_multiGestureProcess) 
	{
		return;
	}
	if (_isMouseScrolling && action->getDetails()->type == SDL_MOUSEMOTION)
	{
		// The following is the workaround for a rare problem where sometimes
		// the mouse-release event is missed for any reason.
		// (checking: is the dragScroll-mouse-button still pressed?)
		// However if the SDL is also missed the release event, then it is to no avail :(
		if ((SDL_GetMouseState(0,0)&SDL_BUTTON(Options::battleDragScrollButton)) == 0)
		{ // so we missed again the mouse-release :(
			// Check if we have to revoke the scrolling, because it was too short in time, so it was a click
			if ((!_mouseMovedOverThreshold) && ((int)(SDL_GetTicks() - _mouseScrollingStartTime) <= (Options::dragScrollTimeTolerance)))
			{
				_map->getCamera()->setMapOffset(_mapOffsetBeforeMouseScrolling);
			}
			_isMouseScrolled = _isMouseScrolling = false;
			stopScrolling(action);
			return;
		}

		_isMouseScrolled = true;

		// Set the mouse cursor back
		SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);
		//assert (0 && "FIXME");
#ifndef __ANDROID__
		/* FIXME: Mouse warping still doesn't work as intended */
		//SDL_WarpMouseInWindow(NULL, _game->getScreen()->getWidth() / 2, _game->getScreen()->getHeight() / 2 - Map::ICON_HEIGHT / 2);
#endif
		SDL_EventState(SDL_MOUSEMOTION, SDL_ENABLE);

		// Check the threshold
		_totalMouseMoveX += action->getDetails()->motion.xrel;
		_totalMouseMoveY += action->getDetails()->motion.yrel;
		if (!_mouseMovedOverThreshold)
		{
			_mouseMovedOverThreshold = ((std::abs(_totalMouseMoveX) > Options::dragScrollPixelTolerance) || (std::abs(_totalMouseMoveY) > Options::dragScrollPixelTolerance));
		}
#ifdef __ANDROID__
		if (_mouseMovedOverThreshold)
		{
			_longPressTimer->stop();
		}
#endif

		// Scrolling
		if (Options::battleDragScrollInvert)
		{
			_map->getCamera()->setMapOffset(_mapOffsetBeforeMouseScrolling);
			int scrollX = -(int)((double)_totalMouseMoveX / action->getXScale());
			int scrollY = -(int)((double)_totalMouseMoveY / action->getYScale());
			Position delta2 = _map->getCamera()->getMapOffset();
			_map->getCamera()->scrollXY(scrollX, scrollY, true);
			delta2 = _map->getCamera()->getMapOffset() - delta2;

			// Keep the limits...
			if (scrollX != delta2.x || scrollY != delta2.y)
			{
				_totalMouseMoveX = -(int) (delta2.x * action->getXScale());
				_totalMouseMoveY = -(int) (delta2.y * action->getYScale());
			}

			action->getDetails()->motion.x = _xBeforeMouseScrolling;
			action->getDetails()->motion.y = _yBeforeMouseScrolling;
			_map->setCursorType(CT_NONE);
		}
		else
		{
			Position delta = _map->getCamera()->getMapOffset();
			_map->getCamera()->setMapOffset(_mapOffsetBeforeMouseScrolling);
			int scrollX = (int)((double)_totalMouseMoveX / action->getXScale());
			int scrollY = (int)((double)_totalMouseMoveY / action->getYScale());
			Position delta2 = _map->getCamera()->getMapOffset();
			_map->getCamera()->scrollXY(scrollX, scrollY, true);
			delta2 = _map->getCamera()->getMapOffset() - delta2;
			delta = _map->getCamera()->getMapOffset() - delta;

			// Keep the limits...
			if (scrollX != delta2.x || scrollY != delta2.y)
			{
				_totalMouseMoveX = (int) (delta2.x * action->getXScale());
				_totalMouseMoveY = (int) (delta2.y * action->getYScale());
			}

			int barWidth = _game->getScreen()->getCursorLeftBlackBand();
			int barHeight = _game->getScreen()->getCursorTopBlackBand();
			int cursorX = _cursorPosition.x + Round(delta.x * action->getXScale());
			int cursorY = _cursorPosition.y + Round(delta.y * action->getYScale());
			_cursorPosition.x = std::min(_game->getScreen()->getWidth() - barWidth - (int)(Round(action->getXScale())), std::max(barWidth, cursorX));
			_cursorPosition.y = std::min(_game->getScreen()->getHeight() - barHeight - (int)(Round(action->getYScale())), std::max(barHeight, cursorY));
			action->getDetails()->motion.x = _cursorPosition.x;
			action->getDetails()->motion.y = _cursorPosition.y;
		}

		// We don't want to look the mouse-cursor jumping :)
		_game->getCursor()->handle(action);
	}
}

/**
 * Processes any presses on the map.
 * @param action Pointer to an action.
 */
void BattlescapeState::mapPress(Action *action)
{
	// don't handle mouseclicks over the buttons (it overlaps with map surface)
	if (_mouseOverIcons) return;

	if (action->getDetails()->button.button == Options::battleDragScrollButton)
	{
		_isMouseScrolling = true;
		_isMouseScrolled = false;
		SDL_GetMouseState(&_xBeforeMouseScrolling, &_yBeforeMouseScrolling);
		_mapOffsetBeforeMouseScrolling = _map->getCamera()->getMapOffset();
		if (!Options::battleDragScrollInvert && _cursorPosition.z == 0)
		{
			_cursorPosition.x = action->getDetails()->button.x;
			_cursorPosition.y = action->getDetails()->button.y;
			// the Z is irrelevant to our mouse position, but we can use it as a boolean to check if the position is set or not
			_cursorPosition.z = 1;
		}
		_totalMouseMoveX = 0; _totalMouseMoveY = 0;
		_mouseMovedOverThreshold = false;
		_mouseScrollingStartTime = SDL_GetTicks();
	}

#ifdef __ANDROID__
	_longPressTimer->start();
	Position pos;
	/* Avoid null pointer dereference */
	BattleUnit *selectedUnit = _save->getSelectedUnit();
	if (selectedUnit != NULL)
	{
		_map->getSelectorPosition(&pos);
		if (pos == selectedUnit->getPosition())
		{
			_swipeFromSoldier = true;
		}
		else
		{
			_swipeFromSoldier = false;
		}
	}
	/* Avoid null pointer in the mapRelease function */
	else
	{
		_swipeFromSoldier = false;
	}
#endif
}

/**
 * Turns the soldier in the swipe direction.
 * @param action Pointer to an action.
 */

void BattlescapeState::mapRelease(Action *action)
{
	_multiGestureProcess = false;
#ifdef __ANDROID__
	_longPressTimer->stop();
	Position pos;
	_map->getSelectorPosition(&pos);
	if (_swipeFromSoldier)
	{
		if (pos != _save->getSelectedUnit()->getPosition())
			_battleGame->secondaryAction(pos);
		_swipeFromSoldier = false;
	}
#endif
}

#ifdef __ANDROID__
/**
 * Handles long presses to turn the soldier.
 */
void BattlescapeState::mapLongPress()
{
	Position pos;
	BattleUnit *selectedUnit = _save->getSelectedUnit();
	_map->getSelectorPosition(&pos);
	if (selectedUnit)
	{
		if (pos != selectedUnit->getPosition())
		{
			_battleGame->secondaryAction(pos);
		}
	}
	_longPressTimer->stop();
}
#endif

/**
 * Processes any clicks on the map to
 * command units.
 * @param action Pointer to an action.
 */
void BattlescapeState::mapClick(Action *action)
{
	// The following is the workaround for a rare problem where sometimes
	// the mouse-release event is missed for any reason.
	// However if the SDL is also missed the release event, then it is to no avail :(
	// (this part handles the release if it is missed and now an other button is used)
	if (_isMouseScrolling)
	{
		if (action->getDetails()->button.button != Options::battleDragScrollButton
		&& (SDL_GetMouseState(0,0)&SDL_BUTTON(Options::battleDragScrollButton)) == 0)
		{   // so we missed again the mouse-release :(
			// Check if we have to revoke the scrolling, because it was too short in time, so it was a click
			if ((!_mouseMovedOverThreshold) && ((int)(SDL_GetTicks() - _mouseScrollingStartTime) <= (Options::dragScrollTimeTolerance)))
			{
				_map->getCamera()->setMapOffset(_mapOffsetBeforeMouseScrolling);
			}
			_isMouseScrolled = _isMouseScrolling = false;
			stopScrolling(action);
		}
	}

	// DragScroll-Button release: release mouse-scroll-mode
	if (_isMouseScrolling)
	{
		// While scrolling, other buttons are ineffective
		if (action->getDetails()->button.button == Options::battleDragScrollButton)
		{
			_isMouseScrolling = false;
			stopScrolling(action);
		}
		else
		{
			return;
		}
		// Check if we have to revoke the scrolling, because it was too short in time, so it was a click
		if ((!_mouseMovedOverThreshold) && ((int)(SDL_GetTicks() - _mouseScrollingStartTime) <= (Options::dragScrollTimeTolerance)))
		{
			_isMouseScrolled = false;
			stopScrolling(action);
		}
		if (_isMouseScrolled) return;
	}

	// right-click aborts walking state
	if (action->getDetails()->button.button == SDL_BUTTON_RIGHT)
	{
		if (_battleGame->cancelCurrentAction())
		{
			return;
		}
	}
	
	// don't handle mouseclicks over the buttons (it overlaps with map surface)
	if (_mouseOverIcons) return;


	// don't accept leftclicks if there is no cursor or there is an action busy
	if (_map->getCursorType() == CT_NONE || _battleGame->isBusy()) return;

	Position pos;
	_map->getSelectorPosition(&pos);

	if (_save->getDebugMode())
	{
		std::wostringstream ss;
		ss << L"Clicked " << pos;
		debug(ss.str());
	}

	if (_save->getTile(pos) != 0) // don't allow to click into void
	{
		if ((action->getDetails()->button.button == SDL_BUTTON_RIGHT || (action->getDetails()->button.button == SDL_BUTTON_LEFT && (SDL_GetModState() & KMOD_ALT) != 0)) && playableUnitSelected())
		{
			_battleGame->secondaryAction(pos);
		}
		else if (action->getDetails()->button.button == SDL_BUTTON_LEFT)
		{
			_battleGame->primaryAction(pos);
		}
	}
}

/**
 * Handles mouse entering the map surface.
 * @param action Pointer to an action.
 */
void BattlescapeState::mapIn(Action *)
{
	_isMouseScrolling = false;
	_map->setButtonsPressed(Options::battleDragScrollButton, false);
}

void BattlescapeState::fingerMotion(Action *action)
{
	//don't scroll if we swipe from soldier, which is used to substitute right
	//click in android
	if (_swipeFromSoldier)
		return;
}

/**
 * Handles multi-finger gestures on the battlefield.
 * Currently it is used for changing elevation.
 * @param action Pointer to an action.
 */

void BattlescapeState::multiGesture(Action *action)
{
	const double levelThreshold = 0.1; //Just an arbitrary number
	static double delta;
	static double prevPointY = 0;
	if (!_multiGestureProcess)
	{
		_multiGestureProcess = true;
		delta = 0;
		prevPointY = action->getDetails()->mgesture.y;
#ifdef __ANDROID__
		_longPressTimer->stop();
#endif
		return;
	}
	delta += action->getDetails()->mgesture.y - prevPointY;
	prevPointY = action->getDetails()->mgesture.y;
	if (std::abs(delta) > levelThreshold)
	{
		if (delta > 0) 
		{
		      btnMapUpClick(action);
		}
		else
		{
		      btnMapDownClick(action);
		}
		delta = 0;
	}
}

#ifdef __ANDROID__
void BattlescapeState::mapKey(Action *action)
{
	if (action->getDetails()->key.keysym.scancode == SDL_SCANCODE_AC_BACK)
	{
		_battleGame->cancelCurrentAction();
	}
}
#endif

/**
 * Moves the selected unit up.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnUnitUpClick(Action *)
{
	if (playableUnitSelected() && _save->getPathfinding()->validateUpDown(_save->getSelectedUnit(), _save->getSelectedUnit()->getPosition(), Pathfinding::DIR_UP))
	{
		_battleGame->cancelCurrentAction();
		_battleGame->moveUpDown(_save->getSelectedUnit(), Pathfinding::DIR_UP);
	}
}

/**
 * Moves the selected unit down.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnUnitDownClick(Action *)
{
	if (playableUnitSelected() && _save->getPathfinding()->validateUpDown(_save->getSelectedUnit(), _save->getSelectedUnit()->getPosition(), Pathfinding::DIR_DOWN))
	{
		_battleGame->cancelCurrentAction();
		_battleGame->moveUpDown(_save->getSelectedUnit(), Pathfinding::DIR_DOWN);
	}
}

/**
 * Shows the next map layer.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnMapUpClick(Action *)
{
	if(_save->getSide() == FACTION_PLAYER || _save->getDebugMode())
		_map->getCamera()->up();
}

/**
 * Shows the previous map layer.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnMapDownClick(Action *)
{
	if(_save->getSide() == FACTION_PLAYER || _save->getDebugMode())
		_map->getCamera()->down();
}

/**
 * Shows the minimap.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnShowMapClick(Action *)
{
	//MiniMapState
	if (allowButtons())
		_game->pushState (new MiniMapState (_map->getCamera(), _save));
}

/**
 * Toggles the current unit's kneel/standup status.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnKneelClick(Action *)
{
	if (allowButtons())
	{
		BattleUnit *bu = _save->getSelectedUnit();
		if (bu)
		{
			_battleGame->kneel(bu);
		}

		// update any path preview if unit kneels
		if (_battleGame->getPathfinding()->isPathPreviewed() && bu->isKneeled())
		{
			_battleGame->getPathfinding()->calculate(_battleGame->getCurrentAction()->actor, _battleGame->getCurrentAction()->target);
			_battleGame->getPathfinding()->removePreview();
			_battleGame->getPathfinding()->previewPath();
		}
	}
}

/**
 * Goes to the soldier info screen.
 * Additionally resets TUs for current side in debug mode.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnInventoryClick(Action *)
{
	if (_save->getDebugMode())
	{
		for (std::vector<BattleUnit*>::iterator i = _save->getUnits()->begin(); i != _save->getUnits()->end(); ++i)
			if ((*i)->getFaction() == _save->getSide())
				(*i)->prepareNewTurn();
		updateSoldierInfo();
	}
	if (playableUnitSelected()
		&& (_save->getSelectedUnit()->getArmor()->getSize() == 1 || _save->getDebugMode())
		&& (_save->getSelectedUnit()->getOriginalFaction() == FACTION_PLAYER ||
			_save->getSelectedUnit()->getRankString() != "STR_LIVE_TERRORIST"))
	{
		// clean up the waypoints
		if (_battleGame->getCurrentAction()->type == BA_LAUNCH)
		{
			_battleGame->getCurrentAction()->waypoints.clear();
			_battleGame->getMap()->getWaypoints()->clear();
			showLaunchButton(false);
		}

		_battleGame->getPathfinding()->removePreview();
		_battleGame->cancelCurrentAction(true);

		_game->pushState(new InventoryState(!_save->getDebugMode(), this));
	}
}

/**
 * Centers on the currently selected soldier.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnCenterClick(Action *)
{
	if (playableUnitSelected())
	{
		_map->getCamera()->centerOnPosition(_save->getSelectedUnit()->getPosition());
		_map->refreshSelectorPosition();
	}
}

/**
 * Selects the next soldier.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnNextSoldierClick(Action *)
{
	if (allowButtons())
	{
		selectNextPlayerUnit(true, false);
		_map->refreshSelectorPosition();
	}
}

/**
 * Disables reselection of the current soldier and selects the next soldier.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnNextStopClick(Action *)
{
	if (allowButtons())
		selectNextPlayerUnit(true, true);
}

/**
 * Selects next soldier.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnPrevSoldierClick(Action *)
{
	if (allowButtons())
	{
		selectPreviousPlayerUnit(true);
		_map->refreshSelectorPosition();
	}
}

/**
 * Selects the next soldier.
 * @param checkReselect When true, don't select a unit that has been previously flagged.
 * @param setReselect When true, flag the current unit first.
 * @param checkInventory When true, don't select a unit that has no inventory.
 */
void BattlescapeState::selectNextPlayerUnit(bool checkReselect, bool setReselect, bool checkInventory)
{
	if (allowButtons())
	{
		if (_battleGame->getCurrentAction()->type != BA_NONE) return;
		BattleUnit *unit = _save->selectNextPlayerUnit(checkReselect, setReselect, checkInventory);
		updateSoldierInfo();
		if (unit) _map->getCamera()->centerOnPosition(unit->getPosition());
		_battleGame->cancelCurrentAction();
		_battleGame->getCurrentAction()->actor = unit;
		_battleGame->setupCursor();
	}
}

/**
 * Selects the previous soldier.
 * @param checkReselect When true, don't select a unit that has been previously flagged.
 * @param setReselect When true, flag the current unit first.
 * @param checkInventory When true, don't select a unit that has no inventory.
 */
void BattlescapeState::selectPreviousPlayerUnit(bool checkReselect, bool setReselect, bool checkInventory)
{
	if (allowButtons())
	{
		if (_battleGame->getCurrentAction()->type != BA_NONE) return;
		BattleUnit *unit = _save->selectPreviousPlayerUnit(checkReselect, setReselect, checkInventory);
		updateSoldierInfo();
		if (unit) _map->getCamera()->centerOnPosition(unit->getPosition());
		_battleGame->cancelCurrentAction();
		_battleGame->getCurrentAction()->actor = unit;
		_battleGame->setupCursor();
	}
}
/**
 * Shows/hides all map layers.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnShowLayersClick(Action *)
{
	_numLayers->setValue(_map->getCamera()->toggleShowAllLayers());
}

/**
 * Shows options.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnHelpClick(Action *)
{
	if (allowButtons(true))
		_game->pushState(new PauseState(OPT_BATTLESCAPE));
}

/**
 * Requests the end of turn. This will add a 0 to the end of the state queue,
 * so all ongoing actions, like explosions are finished first before really switching turn.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnEndTurnClick(Action *)
{
	if (allowButtons())
	{
		_txtTooltip->setText(L"");
		_battleGame->requestEndTurn();
	}
}
/**
 * Aborts the game.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnAbortClick(Action *)
{
	if (allowButtons())
		_game->pushState(new AbortMissionState(_save, this));
}

/**
 * Shows the selected soldier's info.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnStatsClick(Action *action)
{
	if (playableUnitSelected())
	{
		bool b = true;
		if (SCROLL_TRIGGER == Options::battleEdgeScroll &&
			SDL_MOUSEBUTTONUP == action->getDetails()->type && SDL_BUTTON_LEFT == action->getDetails()->button.button)
		{
			int posX = action->getXMouse();
			int posY = action->getYMouse();
			if ((posX < (Camera::SCROLL_BORDER * action->getXScale()) && posX > 0)
				|| (posX > (_map->getWidth() - Camera::SCROLL_BORDER) * action->getXScale())
				|| (posY < (Camera::SCROLL_BORDER * action->getYScale()) && posY > 0)
				|| (posY > (_map->getHeight() - Camera::SCROLL_BORDER) * action->getYScale()))
				// To avoid handling this event as a click
				// on the stats button when the mouse is on the scroll-border
				b = false;
		}
		// clean up the waypoints
		if (_battleGame->getCurrentAction()->type == BA_LAUNCH)
		{
			_battleGame->getCurrentAction()->waypoints.clear();
			_battleGame->getMap()->getWaypoints()->clear();
			showLaunchButton(false);
		}

		_battleGame->cancelCurrentAction(true);

		if (b) popup(new UnitInfoState(_save->getSelectedUnit(), this, false, false));
	}
}

/**
 * Shows an action popup menu. When clicked, creates the action.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnLeftHandItemClick(Action *)
{
	if (playableUnitSelected())
	{
		// concession for touch devices:
		// click on the item to cancel action, and don't pop up a menu to select a new one
		// TODO: wrap this in an IFDEF ?
		if (_battleGame->getCurrentAction()->targeting)
		{
			_battleGame->cancelCurrentAction();
#ifdef __ANDROID__
			if (!_battleGame->getCurrentAction()->targeting)
			{
				_leftWpnActive->setVisible(false);
				_rightWpnActive->setVisible(false);
			}
#endif
			return;
		}

		_battleGame->cancelCurrentAction();

		_save->getSelectedUnit()->setActiveHand("STR_LEFT_HAND");
		_map->cacheUnits();
		_map->draw();
		BattleItem *leftHandItem = _save->getSelectedUnit()->getItem("STR_LEFT_HAND");
		handleItemClick(leftHandItem);
	}
}

/**
 * Shows an action popup menu. When clicked, create the action.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnRightHandItemClick(Action *)
{
	if (playableUnitSelected())
	{
		// concession for touch devices:
		// click on the item to cancel action, and don't pop up a menu to select a new one
		// TODO: wrap this in an IFDEF ?
		if (_battleGame->getCurrentAction()->targeting)
		{
			_battleGame->cancelCurrentAction();
#ifdef __ANDROID__
			if (!_battleGame->getCurrentAction()->targeting)
			{
				_leftWpnActive->setVisible(false);
				_rightWpnActive->setVisible(false);
			}
#endif
			return;
		}

		_battleGame->cancelCurrentAction();

		_save->getSelectedUnit()->setActiveHand("STR_RIGHT_HAND");
		_map->cacheUnits();
		_map->draw();
		BattleItem *rightHandItem = _save->getSelectedUnit()->getItem("STR_RIGHT_HAND");
		handleItemClick(rightHandItem);
	}
}

/**
 * Centers on the unit corresponding to this button.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnVisibleUnitClick(Action *action)
{
	int btnID = -1;

	// got to find out which button was pressed
	for (int i = 0; i < VISIBLE_MAX && btnID == -1; ++i)
	{
		if (action->getSender() == _btnVisibleUnit[i])
		{
			btnID = i;
		}
	}

	if (btnID != -1)
	{
		_map->getCamera()->centerOnPosition(_visibleUnit[btnID]->getPosition());
	}

	action->getDetails()->type = SDL_FIRSTEVENT; // consume the event
#ifdef __ANDROID__
	_longPressTimer->stop();
#endif
}

/**
 * Launches the blaster bomb.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnLaunchClick(Action *action)
{
	_battleGame->launchAction();
	action->getDetails()->type = SDL_FIRSTEVENT; // consume the event
#ifdef __ANDROID__
	// Seems we have an issue with a timer.
	_longPressTimer->stop();
#endif
}

/**
 * Uses psionics.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnPsiClick(Action *action)
{
	_battleGame->psiButtonAction();
	action->getDetails()->type = SDL_FIRSTEVENT; // consume the event
#ifdef __ANDROID__
	_longPressTimer->stop();
#endif
}

/**
 * Reserves time units.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnReserveClick(Action *action)
{
	if (allowButtons())
	{
		SDL_Event ev;
		ev.type = SDL_MOUSEBUTTONDOWN;
		ev.button.button = SDL_BUTTON_LEFT;
		Action a = Action(&ev, 0.0, 0.0, 0, 0);
		action->getSender()->mousePress(&a, this);

		if (_reserve == _btnReserveNone)
			_battleGame->setTUReserved(BA_NONE, true);
		else if (_reserve == _btnReserveSnap)
			_battleGame->setTUReserved(BA_SNAPSHOT, true);
		else if (_reserve == _btnReserveAimed)
			_battleGame->setTUReserved(BA_AIMEDSHOT, true);
		else if (_reserve == _btnReserveAuto)
			_battleGame->setTUReserved(BA_AUTOSHOT, true);

		// update any path preview
		if (_battleGame->getPathfinding()->isPathPreviewed())
		{
			_battleGame->getPathfinding()->removePreview();
			_battleGame->getPathfinding()->previewPath();
		}
	}
}

/**
 * Reloads the weapon in hand.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnReloadClick(Action *)
{
	if (playableUnitSelected() && _save->getSelectedUnit()->checkAmmo())
	{
		_game->getResourcePack()->getSound("BATTLE.CAT", 17)->play();
		updateSoldierInfo();
	}
}

/**
 * Toggles soldier's personal lighting (purely cosmetic).
 * @param action Pointer to an action.
 */
void BattlescapeState::btnPersonalLightingClick(Action *)
{
	if (allowButtons())
		_save->getTileEngine()->togglePersonalLighting();
}

/**
 * Determines whether a playable unit is selected. Normally only player side units can be selected, but in debug mode one can play with aliens too :)
 * Is used to see if stats can be displayed and action buttons will work.
 * @return Whether a playable unit is selected.
 */
bool BattlescapeState::playableUnitSelected()
{
	return _save->getSelectedUnit() != 0 && allowButtons();
}

/**
 * Updates a soldier's name/rank/tu/energy/health/morale.
 */
void BattlescapeState::updateSoldierInfo()
{
	BattleUnit *battleUnit = _save->getSelectedUnit();

	for (int i = 0; i < VISIBLE_MAX; ++i)
	{
		_btnVisibleUnit[i]->setVisible(false);
		_numVisibleUnit[i]->setVisible(false);
		_visibleUnit[i] = 0;
	}

	bool playableUnit = playableUnitSelected();
	_rank->setVisible(playableUnit);
	_numTimeUnits->setVisible(playableUnit);
	_barTimeUnits->setVisible(playableUnit);
	_barTimeUnits->setVisible(playableUnit);
	_numEnergy->setVisible(playableUnit);
	_barEnergy->setVisible(playableUnit);
	_barEnergy->setVisible(playableUnit);
	_numHealth->setVisible(playableUnit);
	_barHealth->setVisible(playableUnit);
	_barHealth->setVisible(playableUnit);
	_numMorale->setVisible(playableUnit);
	_barMorale->setVisible(playableUnit);
	_barMorale->setVisible(playableUnit);
	_btnLeftHandItem->setVisible(playableUnit);
	_btnRightHandItem->setVisible(playableUnit);
	_numAmmoLeft->setVisible(playableUnit);
	_numAmmoRight->setVisible(playableUnit);
	if (!playableUnit)
	{
#ifdef __ANDROID__
		_leftWpnActive->setVisible(false);
		_rightWpnActive->setVisible(false);
#endif
		_txtName->setText(L"");
		showPsiButton(false);
		return;
	}

	_txtName->setText(battleUnit->getName(_game->getLanguage(), false));
	Soldier *soldier = _game->getSavedGame()->getSoldier(battleUnit->getId());
	if (soldier != 0)
	{
		SurfaceSet *texture = _game->getResourcePack()->getSurfaceSet("BASEBITS.PCK");
		texture->getFrame(soldier->getRankSprite())->blit(_rank);
	}
	else
	{
		_rank->clear();
	}
	_numTimeUnits->setValue(battleUnit->getTimeUnits());
	_barTimeUnits->setMax(battleUnit->getStats()->tu);
	_barTimeUnits->setValue(battleUnit->getTimeUnits());
	_numEnergy->setValue(battleUnit->getEnergy());
	_barEnergy->setMax(battleUnit->getStats()->stamina);
	_barEnergy->setValue(battleUnit->getEnergy());
	_numHealth->setValue(battleUnit->getHealth());
	_barHealth->setMax(battleUnit->getStats()->health);
	_barHealth->setValue(battleUnit->getHealth());
	_barHealth->setValue2(battleUnit->getStunlevel());
	_numMorale->setValue(battleUnit->getMorale());
	_barMorale->setMax(100);
	_barMorale->setValue(battleUnit->getMorale());

	BattleItem *leftHandItem = battleUnit->getItem("STR_LEFT_HAND");
	_btnLeftHandItem->clear();
	_numAmmoLeft->setVisible(false);
	if (leftHandItem)
	{
		leftHandItem->getRules()->drawHandSprite(_game->getResourcePack()->getSurfaceSet("BIGOBS.PCK"), _btnLeftHandItem);
		if (leftHandItem->getRules()->getBattleType() == BT_FIREARM && (leftHandItem->needsAmmo() || leftHandItem->getRules()->getClipSize() > 0))
		{
			_numAmmoLeft->setVisible(true);
			if (leftHandItem->getAmmoItem())
				_numAmmoLeft->setValue(leftHandItem->getAmmoItem()->getAmmoQuantity());
			else
				_numAmmoLeft->setValue(0);
		}
	}
	BattleItem *rightHandItem = battleUnit->getItem("STR_RIGHT_HAND");
	_btnRightHandItem->clear();
	_numAmmoRight->setVisible(false);
	if (rightHandItem)
	{
		rightHandItem->getRules()->drawHandSprite(_game->getResourcePack()->getSurfaceSet("BIGOBS.PCK"), _btnRightHandItem);
		if (rightHandItem->getRules()->getBattleType() == BT_FIREARM && (rightHandItem->needsAmmo() || rightHandItem->getRules()->getClipSize() > 0))
		{
			_numAmmoRight->setVisible(true);
			if (rightHandItem->getAmmoItem())
				_numAmmoRight->setValue(rightHandItem->getAmmoItem()->getAmmoQuantity());
			else
				_numAmmoRight->setValue(0);
		}
	}

#ifdef __ANDROID__
	if (_battleGame->getCurrentAction()->targeting)
	{
		BattleItem *currentItem = _battleGame->getCurrentAction()->weapon;
		if (currentItem == leftHandItem)
		{
			_leftWpnActive->setVisible(true);
		}
		else if (currentItem == rightHandItem)
		{
			_rightWpnActive->setVisible(true);
		}
	}
	else
	{
		_leftWpnActive->setVisible(false);
		_rightWpnActive->setVisible(false);
	}
#endif

	_save->getTileEngine()->calculateFOV(_save->getSelectedUnit());
	int j = 0;
	for (std::vector<BattleUnit*>::iterator i = battleUnit->getVisibleUnits()->begin(); i != battleUnit->getVisibleUnits()->end() && j < VISIBLE_MAX; ++i)
	{
		_btnVisibleUnit[j]->setVisible(true);
		_numVisibleUnit[j]->setVisible(true);
		_visibleUnit[j] = (*i);
		++j;
	}

	showPsiButton(battleUnit->getOriginalFaction() == FACTION_HOSTILE && battleUnit->getStats()->psiSkill > 0);
}

/**
 * Shifts the red colors of the visible unit buttons backgrounds.
 */
void BattlescapeState::blinkVisibleUnitButtons()
{
	static int delta = 1, color = 32;

	for (int i = 0; i < VISIBLE_MAX;  ++i)
	{
		if (_btnVisibleUnit[i]->getVisible() == true)
		{
			_btnVisibleUnit[i]->drawRect(0, 0, 15, 12, 15);
			_btnVisibleUnit[i]->drawRect(1, 1, 13, 10, color);
		}
	}

	if (color == 44) delta = -2;
	if (color == 32) delta = 1;

	color += delta;
}

/**
 * Popups a context sensitive list of actions the user can choose from.
 * Some actions result in a change of gamestate.
 * @param item Item the user clicked on (righthand/lefthand)
 */
void BattlescapeState::handleItemClick(BattleItem *item)
{
	// make sure there is an item, and the battlescape is in an idle state
	if (item && !_battleGame->isBusy())
	{
		if (_game->getSavedGame()->isResearched(item->getRules()->getRequirements()) || _save->getSelectedUnit()->getOriginalFaction() == FACTION_HOSTILE)
		{
			_battleGame->getCurrentAction()->weapon = item;
#ifdef __ANDROID__
		BattleUnit *battleUnit = _save->getSelectedUnit();
		if (battleUnit)
		{
			BattleItem *leftHandItem = battleUnit->getItem("STR_LEFT_HAND");
			BattleItem *rightHandItem = battleUnit->getItem("STR_RIGHT_HAND");
			if (item == leftHandItem)
			{
				_leftWpnActive->setVisible(true);
			}
			else
			{
				_leftWpnActive->setVisible(false);
			}
			if (item == rightHandItem)
			{
				_rightWpnActive->setVisible(true);
			}
			else
			{
				_rightWpnActive->setVisible(false);
			}
		}
#endif
			popup(new ActionMenuState(_battleGame->getCurrentAction(), _icons->getX(), _icons->getY()+16));
		}
		else
		{
			warning("STR_UNABLE_TO_USE_ALIEN_ARTIFACT_UNTIL_RESEARCHED");
		}
	}
}

/**
 * Animates map objects on the map, also smoke,fire, ...
 */
void BattlescapeState::animate()
{
	_map->animate(!_battleGame->isBusy());

	blinkVisibleUnitButtons();
}

/**
 * Handles the battle game state.
 */
void BattlescapeState::handleState()
{
	_battleGame->handleState();
}

/**
 * Sets the timer interval for think() calls of the state.
 * @param interval An interval in ms.
 */
void BattlescapeState::setStateInterval(Uint32 interval)
{
	_gameTimer->setInterval(interval);
}

/**
 * Gets pointer to the game. Some states need this info.
 * @return Pointer to game.
 */
Game *BattlescapeState::getGame() const
{
	return _game;
}

/**
 * Gets pointer to the map. Some states need this info.
 * @return Pointer to map.
 */
Map *BattlescapeState::getMap() const
{
	return _map;
}

/**
 * Shows a debug message in the topleft corner.
 * @param message Debug message.
 */
void BattlescapeState::debug(const std::wstring &message)
{
	if (_save->getDebugMode())
	{
		_txtDebug->setText(message);
	}
}

/**
 * Shows a warning message.
 * @param message Warning message.
 */
void BattlescapeState::warning(const std::string &message)
{
	_warning->showMessage(tr(message));
}

/**
 * Takes care of any events from the core game engine.
 * @param action Pointer to an action.
 */
inline void BattlescapeState::handle(Action *action)
{
	if (_game->getCursor()->getVisible() || ((action->getDetails()->type == SDL_MOUSEBUTTONDOWN || action->getDetails()->type == SDL_MOUSEBUTTONUP) && action->getDetails()->button.button == SDL_BUTTON_RIGHT))
	{
		State::handle(action);

		if (_isMouseScrolling && !Options::battleDragScrollInvert)
		{
			_map->setSelectorPosition((_cursorPosition.x - _game->getScreen()->getCursorLeftBlackBand()) / action->getXScale(), (_cursorPosition.y - _game->getScreen()->getCursorTopBlackBand()) / action->getYScale());
		}

		if (action->getDetails()->type == SDL_MOUSEBUTTONDOWN)
		{
			if (action->getDetails()->button.button == SDL_BUTTON_X1)
			{
				btnNextSoldierClick(action);
			}
			else if (action->getDetails()->button.button == SDL_BUTTON_X2)
			{
				btnPrevSoldierClick(action);
			}
		}

		if (action->getDetails()->type == SDL_KEYDOWN)
		{
			if (Options::debug)
			{
				// "ctrl-d" - enable debug mode
				if (action->getDetails()->key.keysym.sym == SDLK_d && (SDL_GetModState() & KMOD_CTRL) != 0)
				{
					_save->setDebugMode();
					debug(L"Debug Mode");
				}
				// "ctrl-v" - reset tile visibility
				else if (_save->getDebugMode() && action->getDetails()->key.keysym.sym == SDLK_v && (SDL_GetModState() & KMOD_CTRL) != 0)
				{
					debug(L"Resetting tile visibility");
					_save->resetTiles();
				}
				// "ctrl-k" - kill all aliens
				else if (_save->getDebugMode() && action->getDetails()->key.keysym.sym == SDLK_k && (SDL_GetModState() & KMOD_CTRL) != 0)
				{
					debug(L"Influenza bacterium dispersed");
					for (std::vector<BattleUnit*>::iterator i = _save->getUnits()->begin(); i !=_save->getUnits()->end(); ++i)
					{
						if ((*i)->getOriginalFaction() == FACTION_HOSTILE)
						{
							(*i)->instaKill();
							if ((*i)->getTile())
							{
								(*i)->getTile()->setUnit(0);
							}
						}
					}
				}
				// f11 - voxel map dump
				else if (action->getDetails()->key.keysym.sym == SDLK_F11)
				{
					saveVoxelMap();
				}
				// f9 - ai
				else if (action->getDetails()->key.keysym.sym == SDLK_F9 && Options::traceAI)
				{
					saveAIMap();
				}
			}
			// quick save and quick load
			// not works in debug mode to prevent conflict in hotkeys by default
			else if (!_game->getSavedGame()->isIronman())
			{
				if (action->getDetails()->key.keysym.sym == Options::keyQuickSave)
				{
					_game->pushState(new SaveGameState(OPT_BATTLESCAPE, SAVE_QUICK, _palette));
				}
				else if (action->getDetails()->key.keysym.sym == Options::keyQuickLoad)
				{
					_game->pushState(new LoadGameState(OPT_BATTLESCAPE, SAVE_QUICK, _palette));
				}
			}

			// voxel view dump
			if (action->getDetails()->key.keysym.sym == Options::keyBattleVoxelView)
			{
				saveVoxelView();
			}
		}
	}
}

/**
 * Saves a map as used by the AI.
 */
void BattlescapeState::saveAIMap()
{
	Uint32 start = SDL_GetTicks();
	BattleUnit *unit = _save->getSelectedUnit();
	if (!unit) return;

	int w = _save->getMapSizeX();
	int h = _save->getMapSizeY();
	Position pos(unit->getPosition());

	int expMax = 0;

	SDL_Surface *img = SDL_CreateRGBSurface(0, w * 8, h * 8, 24, 0xff, 0xff00, 0xff0000, 0);
	Log(LOG_INFO) << "unit = " << unit->getId();
	memset(img->pixels, 0, img->pitch * img->h);

	Position tilePos(pos);
	SDL_Rect r;
	r.h = 8;
	r.w = 8;

	for (int y = 0; y < h; ++y)
	{
		tilePos.y = y;
		for (int x = 0; x < w; ++x)
		{
			tilePos.x = x;
			Tile *t = _save->getTile(tilePos);

			if (!t) continue;
			if (!t->isDiscovered(2)) continue;

		}
	}

	if (expMax < 100) expMax = 100;

	for (int y = 0; y < h; ++y)
	{
		tilePos.y = y;
		for (int x = 0; x < w; ++x)
		{
			tilePos.x = x;
			Tile *t = _save->getTile(tilePos);

			if (!t) continue;
			if (!t->isDiscovered(2)) continue;

			r.x = x * r.w;
			r.y = y * r.h;

			if (t->getTUCost(MapData::O_FLOOR, MT_FLY) != 255 && t->getTUCost(MapData::O_OBJECT, MT_FLY) != 255)
			{
				SDL_FillRect(img, &r, SDL_MapRGB(img->format, 255, 0, 0x20));
				characterRGBA(img, r.x, r.y,'*' , 0x7f, 0x7f, 0x7f, 0x7f);
			} else
			{
				if (!t->getUnit()) SDL_FillRect(img, &r, SDL_MapRGB(img->format, 0x50, 0x50, 0x50)); // gray for blocked tile
			}

			for (int z = tilePos.z; z >= 0; --z)
			{
				Position pos(tilePos.x, tilePos.y, z);
				t = _save->getTile(pos);
				BattleUnit *wat = t->getUnit();
				if (wat)
				{
					switch(wat->getFaction())
					{
					case FACTION_HOSTILE:
						// #4080C0 is Volutar Blue
						characterRGBA(img, r.x, r.y, (tilePos.z - z) ? 'a' : 'A', 0x40, 0x80, 0xC0, 0xff);
						break;
					case FACTION_PLAYER:
						characterRGBA(img, r.x, r.y, (tilePos.z - z) ? 'x' : 'X', 255, 255, 127, 0xff);
						break;
					case FACTION_NEUTRAL:
						characterRGBA(img, r.x, r.y, (tilePos.z - z) ? 'c' : 'C', 255, 127, 127, 0xff);
						break;
					}
					break;
				}
				pos.z--;
				if (z > 0 && !t->hasNoFloor(_save->getTile(pos))) break; // no seeing through floors
			}

			if (t->getMapData(MapData::O_NORTHWALL) && t->getMapData(MapData::O_NORTHWALL)->getTUCost(MT_FLY) == 255)
			{
				lineRGBA(img, r.x, r.y, r.x+r.w, r.y, 0x50, 0x50, 0x50, 255);
			}

			if (t->getMapData(MapData::O_WESTWALL) && t->getMapData(MapData::O_WESTWALL)->getTUCost(MT_FLY) == 255)
			{
				lineRGBA(img, r.x, r.y, r.x, r.y+r.h, 0x50, 0x50, 0x50, 255);
			}
		}
	}

	std::ostringstream ss;

	ss.str("");
	ss << "z = " << tilePos.z;
	stringRGBA(img, 12, 12, ss.str().c_str(), 0, 0, 0, 0x7f);

	int i = 0;
	do
	{
		ss.str("");
		ss << Options::getUserFolder() << "AIExposure" << std::setfill('0') << std::setw(3) << i << ".png";
		i++;
	}
	while (CrossPlatform::fileExists(ss.str()));


	unsigned error = lodepng::encode(ss.str(), (const unsigned char*)img->pixels, img->w, img->h, LCT_RGB);
	if (error)
	{
		Log(LOG_ERROR) << "Saving to PNG failed: " << lodepng_error_text(error);
	}

	SDL_FreeSurface(img);

	Log(LOG_INFO) << "saveAIMap() completed in " << SDL_GetTicks() - start << "ms.";
}

/**
 * Saves a first-person voxel view of the battlescape.
 */
void BattlescapeState::saveVoxelView()
{
	static const unsigned char pal[30]=
	//			ground		west wall	north wall		object		enem unit						xcom unit	neutr unit
	{0,0,0, 224,224,224,  192,224,255,  255,224,192, 128,255,128, 192,0,255,  0,0,0, 255,255,255,  224,192,0,  255,64,128 };

	BattleUnit * bu = _save->getSelectedUnit();
	if (bu==0) return; //no unit selected
	std::vector<Position> _trajectory;

	double ang_x,ang_y;
	bool black;
	Tile *tile;
	std::ostringstream ss;
	std::vector<unsigned char> image;
	int test;
	Position originVoxel = getBattleGame()->getTileEngine()->getSightOriginVoxel(bu);

	Position targetVoxel,hitPos;
	double dist;
	bool _debug = _save->getDebugMode();
	double dir = ((float)bu->getDirection()+4)/4*M_PI;
	image.clear();
	for (int y = -256+32; y < 256+32; ++y)
	{
		ang_y = (((double)y)/640*M_PI+M_PI/2);
		for (int x = -256; x < 256; ++x)
		{
			ang_x = ((double)x/1024)*M_PI+dir;

			targetVoxel.x=originVoxel.x + (int)(-sin(ang_x)*1024*sin(ang_y));
			targetVoxel.y=originVoxel.y + (int)(cos(ang_x)*1024*sin(ang_y));
			targetVoxel.z=originVoxel.z + (int)(cos(ang_y)*1024);

			_trajectory.clear();
			test = _save->getTileEngine()->calculateLine(originVoxel, targetVoxel, false, &_trajectory, bu, true, !_debug) +1;
			black = true;
			if (test!=0 && test!=6)
			{
				tile = _save->getTile(Position(_trajectory.at(0).x/16, _trajectory.at(0).y/16, _trajectory.at(0).z/24));
				if (_debug
					|| (tile->isDiscovered(0) && test == 2)
					|| (tile->isDiscovered(1) && test == 3)
					|| (tile->isDiscovered(2) && (test == 1 || test == 4))
					|| test==5
					)
				{
					if (test==5)
					{
						if (tile->getUnit())
						{
							if (tile->getUnit()->getFaction()==FACTION_NEUTRAL) test=9;
							else
							if (tile->getUnit()->getFaction()==FACTION_PLAYER) test=8;
						}
						else
						{
							tile = _save->getTile(Position(_trajectory.at(0).x/16, _trajectory.at(0).y/16, _trajectory.at(0).z/24-1));
							if (tile && tile->getUnit())
							{
								if (tile->getUnit()->getFaction()==FACTION_NEUTRAL) test=9;
								else
								if (tile->getUnit()->getFaction()==FACTION_PLAYER) test=8;
							}
						}
					}
					hitPos = Position(_trajectory.at(0).x, _trajectory.at(0).y, _trajectory.at(0).z);
					dist = sqrt((double)((hitPos.x-originVoxel.x)*(hitPos.x-originVoxel.x)
						+ (hitPos.y-originVoxel.y)*(hitPos.y-originVoxel.y)
						+ (hitPos.z-originVoxel.z)*(hitPos.z-originVoxel.z)) );
					black = false;
				}
			}

			if (black)
			{
				dist = 0;
			}
			else
			{
				if (dist>1000) dist=1000;
				if (dist<1) dist=1;
				dist=(1000-(log(dist))*140)/700;//140

				if (hitPos.x%16==15)
				{
					dist*=0.9;
				}
				if (hitPos.y%16==15)
				{
					dist*=0.9;
				}
				if (hitPos.z%24==23)
				{
					dist*=0.9;
				}
				if (dist > 1) dist = 1;
				if (tile) dist *= (16 - (float)tile->getShade())/16;
			}

			image.push_back((int)((float)(pal[test*3+0])*dist));
			image.push_back((int)((float)(pal[test*3+1])*dist));
			image.push_back((int)((float)(pal[test*3+2])*dist));
		}
	}

	int i = 0;
	do
	{
		ss.str("");
		ss << Options::getUserFolder() << "fpslook" << std::setfill('0') << std::setw(3) << i << ".png";
		i++;
	}
	while (CrossPlatform::fileExists(ss.str()));


	unsigned error = lodepng::encode(ss.str(), image, 512, 512, LCT_RGB);
	if (error)
	{
		Log(LOG_ERROR) << "Saving to PNG failed: " << lodepng_error_text(error);
	}

	return;
}

/**
 * Saves each layer of voxels on the bettlescape as a png.
 */
void BattlescapeState::saveVoxelMap()
{
	std::ostringstream ss;
	std::vector<unsigned char> image;
	static const unsigned char pal[30]=
	{255,255,255, 224,224,224,  128,160,255,  255,160,128, 128,255,128, 192,0,255,  255,255,255, 255,255,255,  224,192,0,  255,64,128 };

	Tile *tile;

	for (int z = 0; z < _save->getMapSizeZ()*12; ++z)
	{
		image.clear();

		for (int y = 0; y < _save->getMapSizeY()*16; ++y)
		{
			for (int x = 0; x < _save->getMapSizeX()*16; ++x)
			{
				int test = _save->getTileEngine()->voxelCheck(Position(x,y,z*2),0,0) +1;
				float dist=1;
				if (x%16==15)
				{
					dist*=0.9f;
				}
				if (y%16==15)
				{
					dist*=0.9f;
				}

				if (test == V_OUTOFBOUNDS)
				{
					tile = _save->getTile(Position(x/16, y/16, z/12));
					if (tile->getUnit())
					{
						if (tile->getUnit()->getFaction()==FACTION_NEUTRAL) test=9;
						else
						if (tile->getUnit()->getFaction()==FACTION_PLAYER) test=8;
					}
					else
					{
						tile = _save->getTile(Position(x/16, y/16, z/12-1));
						if (tile && tile->getUnit())
						{
							if (tile->getUnit()->getFaction()==FACTION_NEUTRAL) test=9;
							else
							if (tile->getUnit()->getFaction()==FACTION_PLAYER) test=8;
						}
					}
				}

				image.push_back((int)((float)pal[test*3+0]*dist));
				image.push_back((int)((float)pal[test*3+1]*dist));
				image.push_back((int)((float)pal[test*3+2]*dist));
			}
		}

		ss.str("");
		ss << Options::getUserFolder() << "voxel" << std::setfill('0') << std::setw(2) << z << ".png";

		unsigned error = lodepng::encode(ss.str(), image, _save->getMapSizeX()*16, _save->getMapSizeY()*16, LCT_RGB);
		if (error)
		{
			Log(LOG_ERROR) << "Saving to PNG failed: " << lodepng_error_text(error);
		}
	}
	return;
}

/**
 * Adds a new popup window to the queue
 * (this prevents popups from overlapping).
 * @param state Pointer to popup state.
 */
void BattlescapeState::popup(State *state)
{
	_popups.push_back(state);
}

/**
 * Finishes up the current battle, shuts down the battlescape
 * and presents the debriefing screen for the mission.
 * @param abort Was the mission aborted?
 * @param inExitArea Number of soldiers in the exit area OR number of survivors when battle finished due to either all aliens or objective being destroyed.
 */
void BattlescapeState::finishBattle(bool abort, int inExitArea)
{
	while (!_game->isState(this))
	{
		_game->popState();
	}
	_game->getCursor()->setVisible(true);
	std::string nextStage = "";
	if (_save->getMissionType() != "STR_UFO_GROUND_ASSAULT" && _save->getMissionType() != "STR_UFO_CRASH_RECOVERY")
	{
		nextStage = _game->getRuleset()->getDeployment(_save->getMissionType())->getNextStage();
	}
	if (nextStage != "" && inExitArea)
	{
		// if there is a next mission stage + we have people in exit area OR we killed all aliens, load the next stage
		_popups.clear();
		_save->setMissionType(nextStage);
		BattlescapeGenerator bgen = BattlescapeGenerator(_game);
		bgen.setAlienRace("STR_MIXED");
		bgen.nextStage();
		_game->popState();
		_game->pushState(new BriefingState(0, 0));
	}
	else
	{
		_popups.clear();
		_animTimer->stop();
		_gameTimer->stop();
		_game->popState();
		if (abort || (!abort  && inExitArea == 0))
		{
			// abort was done or no player is still alive
			// this concludes to defeat when in mars or mars landing mission
			if ((_save->getMissionType() == "STR_MARS_THE_FINAL_ASSAULT" || _save->getMissionType() == "STR_MARS_CYDONIA_LANDING") && _game->getSavedGame()->getMonthsPassed() > -1)
			{
				_game->pushState (new DefeatState);
			}
			else
			{
				_game->pushState(new DebriefingState);
			}
		}
		else
		{
			// no abort was done and at least a player is still alive
			// this concludes to victory when in mars mission
			if (_save->getMissionType() == "STR_MARS_THE_FINAL_ASSAULT" && _game->getSavedGame()->getMonthsPassed() > -1)
			{
				_game->pushState (new VictoryState);
			}
			else
			{
				_game->pushState(new DebriefingState);
			}
		}
		_game->getCursor()->setColor(Palette::blockOffset(15)+12);
		_game->getFpsCounter()->setColor(Palette::blockOffset(15)+12);
	}
}

/**
 * Shows the launch button.
 * @param show Show launch button?
 */
void BattlescapeState::showLaunchButton(bool show)
{
	_btnLaunch->setVisible(show);
}

/**
 * Shows the PSI button.
 * @param show Show PSI button?
 */
void BattlescapeState::showPsiButton(bool show)
{
	_btnPsi->setVisible(show);
}

/**
 * Clears mouse-scrolling state (isMouseScrolling).
 */
void BattlescapeState::clearMouseScrollingState()
{
	_isMouseScrolling = false;
}

/**
 * Returns a pointer to the battlegame, in case we need its functions.
 */
BattlescapeGame *BattlescapeState::getBattleGame()
{
	return _battleGame;
}

/**
 * Handler for the mouse moving over the icons, disabling the tile selection cube.
 * @param action Pointer to an action.
 */
void BattlescapeState::mouseInIcons(Action *)
{
	_mouseOverIcons = true;
}

/**
 * Handler for the mouse going out of the icons, enabling the tile selection cube.
 * @param action Pointer to an action.
 */
void BattlescapeState::mouseOutIcons(Action *)
{
	_mouseOverIcons = false;
}

/**
 * Checks if the mouse is over the icons.
 * @return True, if the mouse is over the icons.
 */
bool BattlescapeState::getMouseOverIcons() const
{
	return _mouseOverIcons;
}

/**
 * Determines whether the player is allowed to press buttons.
 * Buttons are disabled in the middle of a shot, during the alien turn,
 * and while a player's units are panicking.
 * The save button is an exception as we want to still be able to save if something
 * goes wrong during the alien turn, and submit the save file for dissection.
 * @param allowSaving True, if the help button was clicked.
 * @return True if the player can still press buttons.
 */
bool BattlescapeState::allowButtons(bool allowSaving) const
{
	return ((allowSaving || _save->getSide() == FACTION_PLAYER || _save->getDebugMode())
		&& (_battleGame->getPanicHandled() || _firstInit )
		&& (_map->getProjectile() == 0));
}

/**
 * Reserves time units for kneeling.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnReserveKneelClick(Action *action)
{
	if (allowButtons())
	{
		SDL_Event ev;
		ev.type = SDL_MOUSEBUTTONDOWN;
		ev.button.button = SDL_BUTTON_LEFT;
		Action a = Action(&ev, 0.0, 0.0, 0, 0);
		action->getSender()->mousePress(&a, this);
		_battleGame->setKneelReserved(!_battleGame->getKneelReserved());

		_btnReserveKneel->toggle(_battleGame->getKneelReserved());

		// update any path preview
		if (_battleGame->getPathfinding()->isPathPreviewed())
		{
			_battleGame->getPathfinding()->removePreview();
			_battleGame->getPathfinding()->previewPath();
		}
	}
}

/**
 * Removes all time units.
 * @param action Pointer to an action.
 */
void BattlescapeState::btnZeroTUsClick(Action *action)
{
	if (allowButtons())
	{
		SDL_Event ev;
		ev.type = SDL_MOUSEBUTTONDOWN;
		ev.button.button = SDL_BUTTON_LEFT;
		Action a = Action(&ev, 0.0, 0.0, 0, 0);
		action->getSender()->mousePress(&a, this);
		if (_battleGame->getSave()->getSelectedUnit())
		{
			_battleGame->getSave()->getSelectedUnit()->setTimeUnits(0);
			updateSoldierInfo();
		}
	}
}

/**
* Shows a tooltip for the appropriate button.
* @param action Pointer to an action.
*/
void BattlescapeState::txtTooltipIn(Action *action)
{
	if (allowButtons() && Options::battleTooltips)
	{
		_currentTooltip = action->getSender()->getTooltip();
		_txtTooltip->setText(tr(_currentTooltip));
	}
}

/**
* Clears the tooltip text.
* @param action Pointer to an action.
*/
void BattlescapeState::txtTooltipOut(Action *action)
{
	if (allowButtons() && Options::battleTooltips)
	{
		if (_currentTooltip == action->getSender()->getTooltip())
		{
			_txtTooltip->setText(L"");
		}
	}
}

/**
 * Updates the scale.
 * @param dX delta of X;
 * @param dY delta of Y;
 */
void BattlescapeState::resize(int &dX, int &dY)
{
	dX = Options::baseXResolution;
	dY = Options::baseYResolution;
	int divisor = 1;
	double pixelRatioY = 1.0;

	if (Options::nonSquarePixelRatio)
	{
		pixelRatioY = 1.2;
	}
	switch (Options::battlescapeScale)
	{
	case SCALE_SCREEN_DIV_3:
		divisor = 3;
		break;
	case SCALE_SCREEN_DIV_2:
		divisor = 2;
		break;
	case SCALE_SCREEN:
		break;
	default:
		dX = 0;
		dY = 0;
		return;
	}

	Options::baseXResolution = std::max(Screen::ORIGINAL_WIDTH, Options::displayWidth / divisor);
	Options::baseYResolution = std::max(Screen::ORIGINAL_HEIGHT, (int)(Options::displayHeight / pixelRatioY / divisor));

	dX = Options::baseXResolution - dX;
	dY = Options::baseYResolution - dY;
	_map->setWidth(Options::baseXResolution);
	_map->setHeight(Options::baseYResolution);
	_map->getCamera()->resize();
	_map->getCamera()->jumpXY(dX/2, dY/2);

	for (std::vector<Surface*>::const_iterator i = _surfaces.begin(); i != _surfaces.end(); ++i)
	{
		if (*i != _map && (*i) != _btnPsi && *i != _btnLaunch && *i != _txtDebug)
		{
			(*i)->setX((*i)->getX() + dX / 2);
			(*i)->setY((*i)->getY() + dY);
		}
		else if (*i != _map && *i != _txtDebug)
		{
			(*i)->setX((*i)->getX() + dX);
		}
	}
}

bool BattlescapeState::hasScrolled() const
{
	return _hasScrolled;
}

/**
 * Move the mouse back to where it started after we finish drag scrolling.
 * @param action Pointer to an action.
 */
void BattlescapeState::stopScrolling(Action *action)
{
#ifndef __ANDROID__
	if (Options::battleDragScrollInvert)
	{
//#ifndef __ANDROID__
		/* FIXME: Mouse warping still doesn't work as intended */
		SDL_WarpMouseInWindow(NULL, _xBeforeMouseScrolling, _yBeforeMouseScrolling);
//#endif
		action->setMouseAction(_xBeforeMouseScrolling, _yBeforeMouseScrolling, _map->getX(), _map->getY());
		_battleGame->setupCursor();
	}
	else
	{
//#ifndef __ANDROID__
		/* FIXME: Mouse warping still doesn't work as intended */
		SDL_WarpMouseInWindow(NULL, _cursorPosition.x, _cursorPosition.y);
//#endif
		action->setMouseAction(_cursorPosition.x/action->getXScale(), _cursorPosition.y/action->getYScale(), _game->getScreen()->getSurface()->getX(), _game->getScreen()->getSurface()->getY());
		_map->setSelectorPosition(_cursorPosition.x / action->getXScale(), _cursorPosition.y / action->getYScale());
	}
#endif
	// reset our "mouse position stored" flag
	_cursorPosition.z = 0;
}

void BattlescapeState::consumeEvent(Action *action)
{
	action->getDetails()->type = SDL_FIRSTEVENT;
}


}
