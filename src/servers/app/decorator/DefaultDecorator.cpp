/*
 * Copyright 2001-2020 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus, superstippi@gmx.de
 *		DarkWyrm, bpmagic@columbus.rr.com
 *		Ryan Leavengood, leavengood@gmail.com
 *		Philippe Saint-Pierre, stpere@gmail.com
 *		John Scipione, jscipione@gmail.com
 *		Ingo Weinhold, ingo_weinhold@gmx.de
 *		Clemens Zeidler, haiku@clemens-zeidler.de
 *		Joseph Groover, looncraz@looncraz.net
 *		Tri-Edge AI
 *		Jacob Secunda, secundja@gmail.com
 */


/*!	Default and fallback decorator for the app_server - the yellow tabs */


#include "DefaultDecorator.h"

#include <cmath>
#include <new>
#include <stdio.h>

#include <Autolock.h>
#include <Debug.h>
#include <Rect.h>
#include <Region.h>
#include <View.h>

#include <WindowPrivate.h>

#include "BitmapDrawingEngine.h"
#include "DesktopSettings.h"
#include "DrawingEngine.h"
#include "DrawState.h"
#include "FontManager.h"
#include "ServerBitmap.h"


//#define DEBUG_DECORATOR
#ifdef DEBUG_DECORATOR
#	define STRACE(x) printf x
#else
#	define STRACE(x) ;
#endif


//	#pragma mark -


// TODO: get rid of DesktopSettings here, and introduce private accessor
//	methods to the Decorator base class
DefaultDecorator::DefaultDecorator(DesktopSettings& settings, BRect rect,
	Desktop* desktop)
	:
	TabDecorator(settings, rect, desktop)
{
	// TODO: If the decorator was created with a frame too small, it should
	// resize itself!

	STRACE(("DefaultDecorator:\n"));
	STRACE(("\tFrame (%.1f,%.1f,%.1f,%.1f)\n",
		rect.left, rect.top, rect.right, rect.bottom));
}


DefaultDecorator::~DefaultDecorator()
{
	STRACE(("DefaultDecorator: ~DefaultDecorator()\n"));
}


// #pragma mark - Public methods


/*!	Returns the frame colors for the specified decorator component.

	The meaning of the color array elements depends on the specified component.
	For some components some array elements are unused.

	\param component The component for which to return the frame colors.
	\param highlight The highlight set for the component.
	\param colors An array of colors to be initialized by the function.
*/
void
DefaultDecorator::GetComponentColors(Component component, uint8 highlight,
	ComponentColors _colors, Decorator::Tab* _tab)
{
	Decorator::Tab* tab = static_cast<Decorator::Tab*>(_tab);
	const bool active = tab != NULL && tab->buttonFocus;
	const rgb_color tabColor = active
		? (rgb_color){23, 25, 27, 255}
		: (rgb_color){48, 51, 54, 255};
	const rgb_color frameColor = active
		? (rgb_color){17, 19, 21, 255}
		: (rgb_color){60, 63, 65, 255};
	const rgb_color textColor = active
		? (rgb_color){241, 241, 239, 255}
		: (rgb_color){181, 183, 184, 255};

	switch (component) {
		case COMPONENT_TAB:
			_colors[COLOR_TAB_FRAME_LIGHT] = frameColor;
			_colors[COLOR_TAB_FRAME_DARK] = frameColor;
			_colors[COLOR_TAB] = tabColor;
			_colors[COLOR_TAB_LIGHT] = tabColor;
			_colors[COLOR_TAB_BEVEL] = tabColor;
			_colors[COLOR_TAB_SHADOW] = (rgb_color){101, 105, 108, 255};
			_colors[COLOR_TAB_TEXT] = textColor;
			break;

		case COMPONENT_CLOSE_BUTTON:
		case COMPONENT_MINIMIZE_BUTTON:
		case COMPONENT_ZOOM_BUTTON:
			_colors[COLOR_BUTTON] = tabColor;
			_colors[COLOR_BUTTON_LIGHT] = active
				? (rgb_color){43, 46, 49, 255}
				: (rgb_color){55, 58, 61, 255};
			break;

		case COMPONENT_LEFT_BORDER:
		case COMPONENT_RIGHT_BORDER:
		case COMPONENT_TOP_BORDER:
		case COMPONENT_BOTTOM_BORDER:
		case COMPONENT_RESIZE_CORNER:
		default:
			for (int32 i = 0; i < 6; i++)
				_colors[i] = frameColor;
			if (highlight == HIGHLIGHT_RESIZE_BORDER)
				_colors[0] = (rgb_color){189, 150, 51, 255};
			break;
	}
}


void
DefaultDecorator::UpdateColors(DesktopSettings& settings)
{
	TabDecorator::UpdateColors(settings);
}


// #pragma mark - Protected methods


void
DefaultDecorator::_DrawFrame(BRect rect)
{
	STRACE(("_DrawFrame(%f,%f,%f,%f)\n", rect.left, rect.top,
		rect.right, rect.bottom));

	if (fTopTab->look == B_NO_BORDER_WINDOW_LOOK)
		return;

	if (fBorderWidth <= 0)
		return;

	const bool active = fTopTab != NULL && IsFocus(fTopTab);
	const rgb_color frameColor = active
		? (rgb_color){17, 19, 21, 255}
		: (rgb_color){60, 63, 65, 255};
	const rgb_color outerLine = active
		? (rgb_color){9, 10, 11, 255}
		: (rgb_color){45, 48, 50, 255};

	// Keep the resize footprint, but render it as quiet, straight geometry.
	if (rect.Intersects(fTopBorder))
		fDrawingEngine->FillRect(fTopBorder, frameColor);
	if (rect.Intersects(fLeftBorder))
		fDrawingEngine->FillRect(fLeftBorder, frameColor);
	if (rect.Intersects(fRightBorder))
		fDrawingEngine->FillRect(fRightBorder, frameColor);
	if (rect.Intersects(fBottomBorder))
		fDrawingEngine->FillRect(fBottomBorder, frameColor);

	// Document windows reserve an interior bottom-right resize footprint.
	// Paint it opaquely on every relevant update so stale client/desktop pixels
	// cannot show through while the window is resized.
	if (fTopTab->look == B_DOCUMENT_WINDOW_LOOK && fResizeRect.IsValid()
		&& rect.Intersects(fResizeRect)) {
		ComponentColors colors;
		_GetComponentColors(COMPONENT_RESIZE_CORNER, colors, fTopTab);
		const bool drawGrip = (fTopTab->flags & B_NOT_RESIZABLE) == 0;
		_DrawResizeKnob(fResizeRect, drawGrip, colors);
	}

	BRect border(fTopBorder.LeftTop(), fBottomBorder.RightBottom());
	if (rect.Intersects(border))
		fDrawingEngine->StrokeRect(border, outerLine);
}


void
DefaultDecorator::_DrawResizeKnob(BRect rect, bool drawGrip,
	const ComponentColors& colors)
{
	if (!rect.IsValid())
		return;

	const bool highlighted = colors[0] != colors[1];
	const rgb_color gripColor = highlighted
		? colors[0] : (rgb_color){92, 96, 98, 255};

	// A solid graphite field closes the decorator footprint. Two straight
	// diagonals retain a quiet but discoverable resize affordance.
	fDrawingEngine->FillRect(rect, colors[1]);
	if (!drawGrip)
		return;

	const float step = max_c(3.0f, floorf(rect.Width() / 4.0f));
	fDrawingEngine->StrokeLine(BPoint(rect.left + step, rect.bottom - 1),
		BPoint(rect.right - 1, rect.top + step), gripColor);
	fDrawingEngine->StrokeLine(BPoint(rect.left + step * 2, rect.bottom - 1),
		BPoint(rect.right - 1, rect.top + step * 2), gripColor);
}


/*!	\brief Actually draws the tab

	This function is called when the tab itself needs drawn. Other items,
	like the window title or buttons, should not be drawn here.

	\param tab The \a tab to update.
	\param rect The area of the \a tab to update.
*/
void
DefaultDecorator::_DrawTab(Decorator::Tab* tab, BRect invalid)
{
	STRACE(("_DrawTab(%.1f,%.1f,%.1f,%.1f)\n",
		invalid.left, invalid.top, invalid.right, invalid.bottom));
	const BRect& tabRect = tab->tabRect;
	// If a window has a tab, this will draw it and any buttons which are
	// in it.
	if (!tabRect.IsValid() || !invalid.Intersects(tabRect))
		return;

	ComponentColors colors;
	_GetComponentColors(COMPONENT_TAB, colors, tab);

	const bool active = tab->buttonFocus;
	const rgb_color outerLine = active
		? (rgb_color){17, 19, 21, 255}
		: (rgb_color){60, 63, 65, 255};
	const rgb_color separator = active
		? (rgb_color){101, 105, 108, 255}
		: (rgb_color){74, 78, 81, 255};
	const rgb_color focusLine = (rgb_color){189, 150, 51, 255};

	// Flat Schwarzpause title surface: no gradient, bevel, or raised edge.
	fDrawingEngine->FillRect(tabRect, colors[COLOR_TAB]);
	fDrawingEngine->StrokeRect(tabRect, outerLine);

	if (tab->look != kLeftTitledWindowLook) {
		fDrawingEngine->StrokeLine(BPoint(tabRect.left + 1, tabRect.bottom),
			BPoint(tabRect.right - 1, tabRect.bottom), separator);
		if (active) {
			fDrawingEngine->StrokeLine(BPoint(tabRect.left + 1, tabRect.top),
				BPoint(tabRect.right - 1, tabRect.top), focusLine);
		}
	} else {
		fDrawingEngine->StrokeLine(BPoint(tabRect.right, tabRect.top + 1),
			BPoint(tabRect.right, tabRect.bottom - 1), separator);
		if (active) {
			fDrawingEngine->StrokeLine(BPoint(tabRect.left, tabRect.top + 1),
				BPoint(tabRect.left, tabRect.bottom - 1), focusLine);
		}
	}

	_DrawTitle(tab, tabRect);

	_DrawButtons(tab, invalid);
}


/*!	\brief Actually draws the title

	The main tasks for this function are to ensure that the decorator draws
	the title only in its own area and drawing the title itself.
	Using B_OP_COPY for drawing the title is recommended because of the marked
	performance hit of the other drawing modes, but it is not a requirement.

	\param tab The \a tab to update.
	\param rect area of the title to update.
*/
void
DefaultDecorator::_DrawTitle(Decorator::Tab* _tab, BRect rect)
{
	STRACE(("_DrawTitle(%f,%f,%f,%f)\n", rect.left, rect.top, rect.right,
		rect.bottom));

	Decorator::Tab* tab = static_cast<Decorator::Tab*>(_tab);

	const BRect& tabRect = tab->tabRect;
	const BRect& zoomRect = tab->zoomRect;

	ComponentColors colors;
	_GetComponentColors(COMPONENT_TAB, colors, tab);

	fDrawingEngine->SetDrawingMode(B_OP_OVER);
	fDrawingEngine->SetHighColor(colors[COLOR_TAB_TEXT]);
	fDrawingEngine->SetFont(fDrawState.Font());

	// figure out position of text
	font_height fontHeight;
	fDrawState.Font().GetHeight(fontHeight);

	BPoint titlePos;
	if (tab->look != kLeftTitledWindowLook) {
		titlePos.x = tabRect.left + tab->textOffset;
		titlePos.y = floorf(((tabRect.top + 2.0) + tabRect.bottom
			+ fontHeight.ascent + fontHeight.descent) / 2.0
			- fontHeight.descent + 0.5);
	} else {
		titlePos.x = floorf(((tabRect.left + 2.0) + tabRect.right
			+ fontHeight.ascent + fontHeight.descent) / 2.0
			- fontHeight.descent + 0.5);
		titlePos.y = zoomRect.IsValid() ? zoomRect.top - tab->textOffset
			: tabRect.bottom - tab->textOffset;
	}

	fDrawingEngine->DrawString(tab->truncatedTitle, tab->truncatedTitleLength,
		titlePos);

	fDrawingEngine->SetDrawingMode(B_OP_COPY);
}


/*!	\brief Actually draws the close button

	Unless a subclass has a particularly large button, it is probably
	unnecessary to check the update rectangle.

	\param _tab The \a tab to update.
	\param direct Draw without double buffering.
	\param rect The area of the button to update.
*/
void
DefaultDecorator::_DrawClose(Decorator::Tab* _tab, bool direct, BRect rect)
{
	STRACE(("_DrawClose(%f,%f,%f,%f)\n", rect.left, rect.top, rect.right,
		rect.bottom));

	Decorator::Tab* tab = static_cast<Decorator::Tab*>(_tab);

	int32 index = (tab->buttonFocus ? 0 : 1) + (tab->closePressed ? 0 : 2);
	ServerBitmap* bitmap = tab->closeBitmaps[index];
	if (bitmap == NULL) {
		bitmap = _GetBitmapForButton(tab, COMPONENT_CLOSE_BUTTON,
			tab->closePressed, rect.IntegerWidth(), rect.IntegerHeight());
		tab->closeBitmaps[index] = bitmap;
	}

	_DrawButtonBitmap(bitmap, direct, rect);
}


/*!	\brief Actually draws the zoom button

	Unless a subclass has a particularly large button, it is probably
	unnecessary to check the update rectangle.

	\param _tab The \a tab to update.
	\param direct Draw without double buffering.
	\param rect The area of the button to update.
*/
void
DefaultDecorator::_DrawZoom(Decorator::Tab* _tab, bool direct, BRect rect)
{
	STRACE(("_DrawZoom(%f,%f,%f,%f)\n", rect.left, rect.top, rect.right,
		rect.bottom));

	if (rect.IntegerWidth() < 1)
		return;

	Decorator::Tab* tab = static_cast<Decorator::Tab*>(_tab);
	int32 index = (tab->buttonFocus ? 0 : 1) + (tab->zoomPressed ? 0 : 2);
	ServerBitmap* bitmap = tab->zoomBitmaps[index];
	if (bitmap == NULL) {
		bitmap = _GetBitmapForButton(tab, COMPONENT_ZOOM_BUTTON,
			tab->zoomPressed, rect.IntegerWidth(), rect.IntegerHeight());
		tab->zoomBitmaps[index] = bitmap;
	}

	_DrawButtonBitmap(bitmap, direct, rect);
}


void
DefaultDecorator::_DrawMinimize(Decorator::Tab* tab, bool direct, BRect rect)
{
	int32 index = (tab->buttonFocus ? 0 : 1)
		+ (tab->minimizePressed ? 0 : 2);
	ServerBitmap* bitmap = tab->minimizeBitmaps[index];
	if (bitmap == NULL) {
		bitmap = _GetBitmapForButton(tab, COMPONENT_MINIMIZE_BUTTON,
			tab->minimizePressed, rect.IntegerWidth(), rect.IntegerHeight());
		tab->minimizeBitmaps[index] = bitmap;
	}

	_DrawButtonBitmap(bitmap, direct, rect);
}


bool
DefaultDecorator::_HasMinimizeButton() const
{
	return true;
}


float
DefaultDecorator::_BorderWidthForLook(window_look look) const
{
	switch ((int)look) {
		case B_MODAL_WINDOW_LOOK:
		case B_TITLED_WINDOW_LOOK:
		case B_DOCUMENT_WINDOW_LOOK:
			return 2;

		case B_FLOATING_WINDOW_LOOK:
		case kLeftTitledWindowLook:
		case B_BORDERED_WINDOW_LOOK:
			return 1;

		default:
			return 0;
	}
}


float
DefaultDecorator::_TabSpacing() const
{
	const float scaleFactor = max_c(fDrawState.Font().Size() / 12.0f, 1.0f);
	if (fTopTab->look == B_FLOATING_WINDOW_LOOK
		|| fTopTab->look == kLeftTitledWindowLook) {
		return 4.2f * scaleFactor;
	}

	return 7.0f * scaleFactor;
}


// #pragma mark - Private methods


void
DefaultDecorator::_DrawButtonBitmap(ServerBitmap* bitmap, bool direct,
	BRect rect)
{
	if (bitmap == NULL)
		return;

	bool copyToFrontEnabled = fDrawingEngine->CopyToFrontEnabled();
	fDrawingEngine->SetCopyToFrontEnabled(direct);
	drawing_mode oldMode;
	fDrawingEngine->SetDrawingMode(B_OP_OVER, oldMode);
	fDrawingEngine->DrawBitmap(bitmap, rect.OffsetToCopy(0, 0), rect);
	fDrawingEngine->SetDrawingMode(oldMode);
	fDrawingEngine->SetCopyToFrontEnabled(copyToFrontEnabled);
}


/*!	\brief Draws a framed rectangle with a gradient.
	\param engine The drawing engine to use.
	\param rect The rectangular area to draw in.
	\param down The rectangle should be drawn recessed or not.
	\param colors A button color array of the colors to be used.
*/
void
DefaultDecorator::_DrawBlendedRect(DrawingEngine* engine, const BRect rect,
	bool down, const ComponentColors& colors)
{
	const rgb_color fillColor = down
		? (rgb_color){15, 17, 18, 255}
		: colors[COLOR_BUTTON];
	engine->FillRect(rect, fillColor);
	engine->StrokeLine(rect.LeftTop(), rect.LeftBottom(),
		(rgb_color){46, 50, 53, 255});
}


ServerBitmap*
DefaultDecorator::_GetBitmapForButton(Decorator::Tab* tab, Component item,
	bool down, int32 width, int32 height)
{
	// TODO: the list of shared bitmaps is never freed
	struct decorator_bitmap {
		Component			item;
		bool				down;
		int32				width;
		int32				height;
		rgb_color			baseColor;
		rgb_color			lightColor;
		UtilityBitmap*		bitmap;
		decorator_bitmap*	next;
	};

	static BLocker sBitmapListLock("decorator lock", true);
	static decorator_bitmap* sBitmapList = NULL;

	ComponentColors colors;
	_GetComponentColors(item, colors, tab);

	BAutolock locker(sBitmapListLock);

	// search our list for a matching bitmap
	// TODO: use a hash map instead?
	decorator_bitmap* current = sBitmapList;
	while (current) {
		if (current->item == item && current->down == down
			&& current->width == width && current->height == height
			&& current->baseColor == colors[COLOR_BUTTON]
			&& current->lightColor == colors[COLOR_BUTTON_LIGHT]) {
			return current->bitmap;
		}

		current = current->next;
	}

	static BitmapDrawingEngine* sBitmapDrawingEngine = NULL;

	// didn't find any bitmap, create a new one
	if (sBitmapDrawingEngine == NULL)
		sBitmapDrawingEngine = new(std::nothrow) BitmapDrawingEngine();
	if (sBitmapDrawingEngine == NULL
		|| sBitmapDrawingEngine->SetSize(width, height) != B_OK)
		return NULL;

	BRect rect(0, 0, width - 1, height - 1);
	const bool active = tab != NULL && tab->buttonFocus;
	const rgb_color closeColor = active
		? (rgb_color){210, 43, 53, 255}
		: (rgb_color){130, 70, 74, 255};
	const rgb_color minimizeColor = active
		? (rgb_color){205, 176, 50, 255}
		: (rgb_color){130, 116, 65, 255};
	const rgb_color zoomColor = active
		? (rgb_color){50, 174, 75, 255}
		: (rgb_color){65, 120, 75, 255};
	const rgb_color zoomBackColor = active
		? (rgb_color){148, 151, 153, 255}
		: (rgb_color){92, 96, 98, 255};
	const float glyphShift = width >= 12 ? 2.0f : 1.0f;

	STRACE(("DefaultDecorator creating bitmap for %s %s at size %ldx%ld\n",
		item == COMPONENT_CLOSE_BUTTON ? "close" : "zoom",
		down ? "down" : "up", width, height));
	switch (item) {
		case COMPONENT_CLOSE_BUTTON:
			_DrawBlendedRect(sBitmapDrawingEngine, rect, down, colors);
			sBitmapDrawingEngine->StrokeLine(BPoint(2 + glyphShift, height - 3),
				BPoint(width - 3 + glyphShift, 2), closeColor);
			break;

		case COMPONENT_MINIMIZE_BUTTON:
			_DrawBlendedRect(sBitmapDrawingEngine, rect, down, colors);
			// Keep a narrow visible gap above the button's bottom frame.
			sBitmapDrawingEngine->StrokeLine(BPoint(2 + glyphShift, height - 4),
				BPoint(width - 3 + glyphShift, height - 4), minimizeColor);
			break;

		case COMPONENT_ZOOM_BUTTON:
		{
			_DrawBlendedRect(sBitmapDrawingEngine, rect, down, colors);
			const float backWidth = max_c(7.0f, floorf(width * 0.58f));
			const float backHeight = max_c(5.0f, floorf(height * 0.50f));
			BRect back(2 + glyphShift, height - backHeight - 2,
				min_c((float)width - 1, 2 + backWidth + glyphShift),
				height - 3);
			const float frontWidth = max_c(5.0f, floorf(width * 0.40f));
			const float frontHeight = max_c(4.0f, floorf(height * 0.38f));
			BRect front(width - frontWidth - 2 + glyphShift, 2,
				width - 3 + glyphShift,
				2 + frontHeight);
			sBitmapDrawingEngine->StrokeRect(back, zoomBackColor);
			// The foreground rectangle masks the rear outline where they overlap.
			sBitmapDrawingEngine->FillRect(front, colors[COLOR_BUTTON]);
			sBitmapDrawingEngine->StrokeRect(front, zoomColor);
			break;
		}

		default:
			break;
	}

	UtilityBitmap* bitmap = sBitmapDrawingEngine->ExportToBitmap(width, height,
		B_RGB32);
	if (bitmap == NULL)
		return NULL;

	// bitmap ready, put it into the list
	decorator_bitmap* entry = new(std::nothrow) decorator_bitmap;
	if (entry == NULL) {
		delete bitmap;
		return NULL;
	}

	entry->item = item;
	entry->down = down;
	entry->width = width;
	entry->height = height;
	entry->bitmap = bitmap;
	entry->baseColor = colors[COLOR_BUTTON];
	entry->lightColor = colors[COLOR_BUTTON_LIGHT];
	entry->next = sBitmapList;
	sBitmapList = entry;

	return bitmap;
}


void
DefaultDecorator::_GetComponentColors(Component component,
	ComponentColors _colors, Decorator::Tab* tab)
{
	// get the highlight for our component
	Region region = REGION_NONE;
	switch (component) {
		case COMPONENT_TAB:
			region = REGION_TAB;
			break;
		case COMPONENT_CLOSE_BUTTON:
			region = REGION_CLOSE_BUTTON;
			break;
		case COMPONENT_MINIMIZE_BUTTON:
			region = REGION_MINIMIZE_BUTTON;
			break;
		case COMPONENT_ZOOM_BUTTON:
			region = REGION_ZOOM_BUTTON;
			break;
		case COMPONENT_LEFT_BORDER:
			region = REGION_LEFT_BORDER;
			break;
		case COMPONENT_RIGHT_BORDER:
			region = REGION_RIGHT_BORDER;
			break;
		case COMPONENT_TOP_BORDER:
			region = REGION_TOP_BORDER;
			break;
		case COMPONENT_BOTTOM_BORDER:
			region = REGION_BOTTOM_BORDER;
			break;
		case COMPONENT_RESIZE_CORNER:
			region = REGION_RIGHT_BOTTOM_CORNER;
			break;
	}

	return GetComponentColors(component, RegionHighlight(region), _colors, tab);
}
