
#include <SDL.h>
#include "UIBuilder.h"
#include "Surface.h"
#include "SurfaceSet.h"
#include "InteractiveSurface.h"
#include "State.h"
#include "Options.h"
#include "../Interface/ImageButton.h"
#include "../Interface/NumberText.h"
#include "../Interface/Bar.h"
#include "../Interface/Text.h"
#include <vector>
#include <algorithm>
#include "Logger.h"
#include "Palette.h"


namespace OpenXcom
{

namespace UIBuilder
{
	/// Map containing event handler names and corresponding method addresses. For internal use.
	static std::map<std::string, ActionHandler> _handlers;
	/// Map containing keyboard shortcut names and corresponding keycodes. For internal use.
	static std::map<std::string, SDL_Keycode> _kbShortcuts;
	/// Pointer to the current game object. Used for some resource management. For internal use.
	static ResourcePack *_resourcePack;
	/// Pointer to the current state. For internal use.
	static State *_currentState;

	/**
	 * Load parameters defined for all types of surfaces.
	 * @param node Reference to a YAML::Node containing the data.
	 * @param out Pointer to a Surface or Surface-derived object.
	 */
	static inline void setCommonParams(const YAML::Node &node, Surface *out)
	{
		std::string hAlign = node["hAlign"].as<std::string>();
		std::string vAlign = node["vAlign"].as<std::string>();
		// Set position for the surface.
		// hAlign may be "left", "center" or "right".
		// "left" means that surface will be shifted 0 pixels to the right,
		// "center" means it will be shifted half the screen to the right,
		// "right" means it will be shifted all the way to the right.
		// In case of "right" alignment you'll have to set relX to a negative number!
		// Likewise, vAlign may be "top", "center" and "bottom", and with "bottom"
		// you have to shift it up by using negative numbers! Braindead, I know.
		int absX, absY;
		
		if (hAlign == "left")
		{
			absX = 0;
		}
		else if (hAlign == "center")
		{
			absX = Options::baseXResolution / 2;
		}
		else if (hAlign == "right")
		{
			absX = Options::baseXResolution;
		}
		else
		{
			Log(LOG_WARNING) << "Unknown hAlign value:" << hAlign << " encountered while parsing " << node["name"] << " of " << node["type"];
		} // end hAlign
		
		if (vAlign == "top")
		{
			absY = 0;
		}
		else if (vAlign == "center")
		{
			absY = Options::baseYResolution / 2;
		}
		else if (vAlign == "bottom")
		{
			absY = Options::baseYResolution;
		}
		else
		{
			Log(LOG_WARNING) << "Unknown vAlign value: " << vAlign << " encountered while parsing " << node["name"] << " of " << node["type"];
		} // end vAlign

		absX += node["relX"].as<int>();
		absY += node["relY"].as<int>();
		out->setX(absX); out->setY(absY);
		
		bool hidden = node["hidden"].as<bool>(false);
		out->setHidden(hidden);
		
		bool visible = node["visible"].as<bool>(true);
		out->setVisible(visible);

		// Always set a palette for the surfaces!
		out->setPalette(_currentState->getPalette());
		// A surface might be cropped; we must handle this too.
		if (node["crop"])
		{
			const YAML::Node &crop = node["crop"];
			SDL_Rect *r = out->getCrop();
			r->x = crop["x"].as<int>();
			r->y = crop["y"].as<int>();
			r->w = crop["w"].as<int>();
			r->h = crop["h"].as<int>();
		}
		
		// A surface might or might not have a graphic; in case it does we should use it.
		if (node["graphic"])
		{
			const YAML::Node &graphic = node["graphic"];
			// Pointer to the surface containing the graphic
			Surface *src;
			// offset for the image (for example, TFTDReserve)
			int offX = 0;
			int offY = 0;
			// name of the graphic - must be defined in the ruleset!
			std::string name = graphic["name"].as<std::string>();
			if (graphic["offsetX"])
			{
				offX = graphic["offsetX"].as<int>();
			}
			if (graphic["offsetY"])
			{
				offY = graphic["offsetY"].as<int>();
			}

			// Some images come from surface sets, others are self-contained.
			if (graphic["frame"])
			{
				int frame = graphic["frame"].as<int>();
				src = _resourcePack->getSurfaceSet(name)->getFrame(frame);
			}
			else
			{
				src = _resourcePack->getSurface(name);
			}
			src->setX(offX);
			src->setY(offY);
			if (graphic["crop"])
			{
				const YAML::Node &crop = graphic["crop"];
				SDL_Rect *r = src->getCrop();
				r->x = crop["x"].as<int>();
				r->y = crop["y"].as<int>();
				r->w = crop["w"].as<int>();
				r->h = crop["h"].as<int>();
			}

			src->blit(out);
		}

		// Finally, some surfaces are actually helpful! We just need to load the tooltip.
		std::string tooltip = node["tooltip"].as<std::string>("");
		out->setTooltip(tooltip);
	}
	/**
	 * Set handlers for InteractiveSurface or InteractiveSurface-derived
	 * objects.
	 * @param node Reference to a YAML::Node containing the handlers.
	 * @param out Pointer to an InteractiveSurface or derived object.
	 */
	static inline void setHandlers(const YAML::Node &node, InteractiveSurface *out)
	{
		// Just in case you're wondering: yes, that's a long string of if()'s
		// that set handlers for the interactive surfaces.
		std::string hndlName;
		std::string keyName;
		hndlName = node["onMouseClick"].as<std::string>("");
		out->onMouseClick(_handlers[hndlName], SDL_BUTTON_LEFT);

		hndlName = node["onMouseRightClick"].as<std::string>("");
		out->onMouseClick(_handlers[hndlName], SDL_BUTTON_RIGHT);

		hndlName = node["onMousePress"].as<std::string>("");
		out->onMousePress(_handlers[hndlName]);

		hndlName = node["onMouseRelease"].as<std::string>("");
		out->onMouseRelease(_handlers[hndlName]);
		
		hndlName = node["onMouseIn"].as<std::string>("");
		out->onMouseIn(_handlers[hndlName]);
		
		hndlName = node["onMouseOver"].as<std::string>("");
		out->onMouseOver(_handlers[hndlName]);

		hndlName = node["onMouseOut"].as<std::string>("");
		out->onMouseOut(_handlers[hndlName]);

		hndlName = node["onKeyboardPress"].as<std::string>("");
		if (node["keyKeyboardPress"])
		{
			keyName = node["keyKeyboardPress"].as<std::string>();
			out->onKeyboardPress(_handlers[hndlName], _kbShortcuts[keyName]);
		}
		else
		{
			out->onKeyboardPress(_handlers[hndlName]);
		}
		
		hndlName = node["onKeyboardRelease"].as<std::string>("");
		if (node["keyKeyboardRelease"])
		{
			keyName = node["keyKeyboardRelease"].as<std::string>();
			out->onKeyboardRelease(_handlers[hndlName], _kbShortcuts[keyName]);
		}
		else
		{
			out->onKeyboardRelease(_handlers[hndlName]);
		}
	}

	/**
	 * Create a new Surface object, set the required properties and return the corresponding pointer.
	 * @param surfNode A YAML::Node object containing the required data.
	 * @return The pointer to a newly created Surface object.
	 */
	static inline Surface* createSurface(const YAML::Node &surfNode)
	{
		int width = surfNode["width"].as<int>();
		int height = surfNode["height"].as<int>();
		Surface *out = new Surface(width, height);
		setCommonParams(surfNode, out);

		return out;
	}

	/**
	 * Create a new InteractiveSurface object, set the required properties and return
	 * the corresponding pointer.
	 * @param isurfNode A YAML::Node object containing the required data.
	 * @return The pointer to a newly created InteractiveSurface object.
	 */
	static inline InteractiveSurface* createISurface(const YAML::Node &isurfNode)
	{
		int width = isurfNode["width"].as<int>();
		int height = isurfNode["height"].as<int>();
		InteractiveSurface *out = new InteractiveSurface(width, height);
		setCommonParams(isurfNode, out);
		if(isurfNode["handlers"])
		{
			setHandlers(isurfNode["handlers"], out);
		}
		return out;
	}

	/**
	 * Create a new ImageButton object, set the required properties and return
	 * the corresponding pointer.
	 * @param isurfNode A YAML::Node object containing the required data.
	 * @return The pointer to a newly created InteractiveSurface object.
	 */
	static inline ImageButton* createIButton(const YAML::Node &ibtnNode)
	{
		int width = ibtnNode["width"].as<int>();
		int height = ibtnNode["height"].as<int>();
		ImageButton *out = new ImageButton(width, height);
		setCommonParams(ibtnNode, out);
		if(ibtnNode["handlers"])
		{
			setHandlers(ibtnNode["handlers"], out);
		}
		// The image button needs a color from the palette.
		const YAML::Node color = ibtnNode["palColor"];
		int blockOffset = color["blockOffset"].as<int>(0);
		int colorOffset = color["colorOffset"].as<int>(0);
		out->setColor(Palette::blockOffset(blockOffset) + colorOffset);
		return out;
	}

	/**
	 * Create a new NumberText object, set the required properties and return
	 * the corresponding pointer.
	 * @param ntxtNode A YAML::Node object containing the required data.
	 * @return The pointer to a newly created NumberText object.
	 */
	static inline NumberText* createNumText(const YAML::Node &ntxtNode)
	{
		int width = ntxtNode["width"].as<int>();
		int height = ntxtNode["height"].as<int>();
		NumberText *out = new NumberText(width, height);
		setCommonParams(ntxtNode, out);

		const YAML::Node &color = ntxtNode["palColor"];
		int blockOffset = color["blockOffset"].as<int>(0);
		int colorOffset = color["colorOffset"].as<int>(0);

		unsigned int value = ntxtNode["value"].as<unsigned int>(0);

		out->setColor(Palette::blockOffset(blockOffset) + colorOffset);
		out->setValue(value);

		return out;
	}

	/**
	 * Create a new Bar object, set the required properties and return
	 * the corresponding pointer.
	 * @param barfNode A YAML::Node object containing the required data.
	 * @return The pointer to a newly created Bar object.
	 */
	static inline Bar* createBar(const YAML::Node &barNode)
	{
		int width = barNode["width"].as<int>();
		int height = barNode["height"].as<int>();
		Bar *out = new Bar(width, height);
		setCommonParams(barNode, out);
		// Load the bar's colors
		const YAML::Node &color1 = barNode["palColor"];
		
		int blockOffset1 = color1["blockOffset"].as<int>(0);
		int colorOffset1 = color1["colorOffset"].as<int>(0);
		out->setColor(Palette::blockOffset(blockOffset1) + colorOffset1);
		if (barNode["palColor2"])
		{
			const YAML::Node &color2 = barNode["palColor2"];
			int blockOffset2 = color2["blockOffset"].as<int>(0);
			int colorOffset2 = color2["colorOffset"].as<int>(0);
			out->setColor2(Palette::blockOffset(blockOffset2) + colorOffset2);
		}
		
		// Load the bar's scale
		double scale = barNode["scale"].as<double>();
		out->setScale(scale);

		return out;
	}

	static inline Text* createText(const YAML::Node &txtNode)
	{
		int width = txtNode["width"].as<int>();
		int height = txtNode["height"].as<int>();
		Text *out = new Text(width, height);
		setCommonParams(txtNode, out);

		const YAML::Node &color = txtNode["palColor"];
		int blockOffset = color["blockOffset"].as<int>(0);
		int colorOffset = color["colorOffset"].as<int>(0);
		out->setColor(Palette::blockOffset(blockOffset) + colorOffset);

		bool highContrast = txtNode["highContrast"].as<bool>(false);
		out->setHighContrast(highContrast);

		return out;
	}


	/// Comparator function for vector sorting
	bool cmp(const std::pair<int, Surface*> &a, const std::pair<int, Surface*> &b)
	{
		return a.first < b.first;
	}


	/**
	 * Loads the layout from a yaml file and returns a map of UI element names and corresponding pointers.
	 * All elements are also added to the current state and will be deleted in the state's destructor.
	 * All pointers are Surfaces* by default, so you'll have to dynamic_cast<> them to whatever type
	 * you're expecting.
	 * @note You NEED to set the state's palette before calling this function!
	 * @note DON'T ADD THE ELEMENTS TO THE STATE, IT'S DONE AUTOMATICALLY!
	 * @param currentState A pointer to the current state.
	 * @param resourcePack A pointer to the ResourcePack object (Is it actually needed, though?).
	 * @param handlers A reference to a map of ActionHandlers. See BattlescapeState.cpp for details.
	 * @param kbShortcuts A reference to a map of SDLKeys for keyboard shortcut assignment.
	 * @param file A reference to a filename string. Should point to a .layout document.
	 * @return A map of UI element names to corresponding UI elements.
	 */
	std::map<std::string, Surface*> buildUI(State *currentState,
				ResourcePack *resourcePack,
				std::map<std::string, ActionHandler> &handlers,
				std::map<std::string, SDL_Keycode> &kbShortcuts, 
				const std::string &file)
	{
		// The thing we'll be returning - a map of UI element names to the corresponding surfaces.
		// There may be more elements within the map than is needed, but they'll be added to the
		// state and disposed of anyway.
		std::map<std::string, Surface*> out;
		// Since some things depend on the order surfaces are added to the state,
		// we make a vector that takes desired element order into account.
		std::vector<std::pair<int, Surface*> > uiElements;

		// Palette must be loaded beforehand!
		assert(currentState->getPalette() != 0);
		Log(LOG_DEBUG) << "Reading UI from " << file;
		_handlers = handlers;
		_kbShortcuts = kbShortcuts;
		_resourcePack = resourcePack;
		_currentState = currentState;

		YAML::Node layout = YAML::LoadFile(file);
		
		for(YAML::const_iterator i = layout["layout"].begin(); i != layout["layout"].end(); ++i)
		{
			Surface *sfc = 0;
			std::string elementName = (*i)["name"].as<std::string>();
			std::string typeString = (*i)["type"].as<std::string>();
			int order = (*i)["order"].as<int>();
			if (typeString == "Surface")
			{
				sfc = createSurface(*i);
			}
			else if (typeString == "InteractiveSurface")
			{
				sfc = createISurface(*i);
			}
			else if (typeString == "ImageButton")
			{
				sfc = createIButton(*i);
			}
			else if (typeString == "NumberText")
			{
				sfc = createNumText(*i);
			}
			else if (typeString == "Bar")
			{
				sfc = createBar(*i);
			}
			else if (typeString == "Text")
			{
				sfc = createText(*i);
			}

			out[elementName] = sfc;
			uiElements.push_back(std::make_pair<int, Surface* >(int(order), (Surface*)sfc));
		}
		std::sort(uiElements.begin(), uiElements.end(), cmp);
		for (std::vector<std::pair<int, Surface*> >::iterator i = uiElements.begin(); i != uiElements.end(); ++i)
		{
			currentState->add(i->second);
		}

		return out;

	}
}

}
