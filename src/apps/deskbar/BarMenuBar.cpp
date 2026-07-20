/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2000, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered
trademarks of Be Incorporated in the United States and other countries. Other
brand product names are registered trademarks or trademarks of their respective
holders.
All rights reserved.
*/


#include "BarMenuBar.h"

#include <algorithm>
#include <new>
#include <string.h>

#include <Bitmap.h>
#include <ControlLook.h>
#include <Debug.h>
#include <Font.h>
#include <IconUtils.h>
#include <NodeInfo.h>
#include <TranslatorFormats.h>
#include <TranslationUtils.h>
#include <View.h>

#include "icons.h"

#include "BarMenuTitle.h"
#include "BarView.h"
#include "BarWindow.h"
#include "DeskbarMenu.h"
#include "DeskbarUtils.h"
#include "ResourceSet.h"
#include "StatusView.h"
#include "TeamMenu.h"


const float kSepItemWidth = 5.0f;

const float kTeamIconBitmapHeight = 19.f;


static BBitmap*
CreateFallbackSchwarzpauseMenuIcon(float width, float height)
{
	BRect bounds(0, 0, width - 1, height - 1);
	BBitmap* icon = new(std::nothrow) BBitmap(bounds, B_RGBA32, true);
	if (icon == NULL || icon->InitCheck() != B_OK) {
		delete icon;
		return NULL;
	}

	if (icon->Bits() != NULL && icon->BitsLength() > 0)
		memset(icon->Bits(), 0, icon->BitsLength());

	if (!icon->Lock()) {
		delete icon;
		return NULL;
	}

	BView* canvas = new(std::nothrow) BView(bounds, "schwarzpause menu icon",
		B_FOLLOW_NONE, B_WILL_DRAW);
	if (canvas == NULL) {
		icon->Unlock();
		delete icon;
		return NULL;
	}
	icon->AddChild(canvas);

	canvas->SetDrawingMode(B_OP_COPY);
	canvas->SetHighColor(0, 0, 0, 0);
	canvas->FillRect(bounds);

	canvas->SetDrawingMode(B_OP_ALPHA);
	canvas->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);

	BRect iconArea = bounds.InsetByCopy(5, 3);
	float unit = std::max(1.0f, iconArea.Height() / 25.0f);
	float potLeft = iconArea.left + 6 * unit;
	float potTop = iconArea.top + 15 * unit;
	BPoint pot[] = {
		BPoint(potLeft, potTop),
		BPoint(potLeft + 12 * unit, potTop - 4 * unit),
		BPoint(potLeft + 19 * unit, potTop + 5 * unit),
		BPoint(potLeft + 14 * unit, potTop + 15 * unit),
		BPoint(potLeft + 3 * unit, potTop + 15 * unit),
		BPoint(potLeft - 4 * unit, potTop + 6 * unit)
	};
	canvas->SetHighColor(2, 3, 5, 245);
	canvas->FillPolygon(pot, 6);
	canvas->SetHighColor(255, 255, 255, 58);
	canvas->StrokeLine(BPoint(potLeft + 3 * unit, potTop + 12 * unit),
		BPoint(potLeft + 14 * unit, potTop + 12 * unit));

	BPoint shaftStart(potLeft + 13 * unit, potTop + 1 * unit);
	BPoint shaftEnd(iconArea.right - 3 * unit, iconArea.top + 4 * unit);
	canvas->SetPenSize(3.0f * unit);
	canvas->SetHighColor(248, 249, 250, 255);
	canvas->StrokeLine(shaftStart, shaftEnd);
	canvas->SetPenSize(1.5f * unit);
	canvas->SetHighColor(230, 233, 238, 245);
	canvas->StrokeLine(BPoint(shaftStart.x + 11 * unit, shaftStart.y - 3 * unit),
		BPoint(shaftStart.x + 21 * unit, shaftStart.y - 10 * unit));
	canvas->StrokeLine(BPoint(shaftStart.x + 18 * unit, shaftStart.y - 5 * unit),
		BPoint(shaftStart.x + 30 * unit, shaftStart.y - 12 * unit));
	canvas->StrokeLine(BPoint(shaftStart.x + 25 * unit, shaftStart.y - 7 * unit),
		BPoint(shaftEnd.x, shaftEnd.y - 1 * unit));
	canvas->StrokeLine(BPoint(shaftStart.x + 16 * unit, shaftStart.y - 4 * unit),
		BPoint(shaftStart.x + 23 * unit, shaftStart.y + 1 * unit));
	canvas->StrokeLine(BPoint(shaftStart.x + 24 * unit, shaftStart.y - 6 * unit),
		BPoint(shaftStart.x + 33 * unit, shaftStart.y - 1 * unit));

	canvas->Sync();
	icon->RemoveChild(canvas);
	icon->Unlock();
	delete canvas;

	return icon;
}


static BBitmap*
CreateSchwarzpauseMenuIcon(float width, float height)
{
	BBitmap* logo = BTranslationUtils::GetBitmap(B_PNG_FORMAT,
		"schwarzpause_start_mark.png");
	if (logo == NULL)
		return CreateFallbackSchwarzpauseMenuIcon(width, height);

	BRect bounds(0, 0, width - 1, height - 1);
	BBitmap* icon = new(std::nothrow) BBitmap(bounds, B_RGBA32, true);
	if (icon == NULL || icon->InitCheck() != B_OK) {
		delete logo;
		delete icon;
		return CreateFallbackSchwarzpauseMenuIcon(width, height);
	}

	if (icon->Bits() != NULL && icon->BitsLength() > 0)
		memset(icon->Bits(), 0, icon->BitsLength());

	if (!icon->Lock()) {
		delete logo;
		delete icon;
		return CreateFallbackSchwarzpauseMenuIcon(width, height);
	}

	BView* canvas = new(std::nothrow) BView(bounds,
		"schwarzpause menu logo", B_FOLLOW_NONE, B_WILL_DRAW);
	if (canvas == NULL) {
		icon->Unlock();
		delete logo;
		delete icon;
		return CreateFallbackSchwarzpauseMenuIcon(width, height);
	}
	icon->AddChild(canvas);

	canvas->SetDrawingMode(B_OP_COPY);
	canvas->SetHighColor(0, 0, 0, 0);
	canvas->FillRect(bounds);

	canvas->SetDrawingMode(B_OP_ALPHA);
	canvas->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);

	BRect source(logo->Bounds());
	BRect target(bounds);
	canvas->DrawBitmapAsync(logo, source, target);

	canvas->Sync();
	icon->RemoveChild(canvas);
	icon->Unlock();
	delete canvas;
	delete logo;

	return icon;
}


//	#pragma mark - TSeparatorItem


TSeparatorItem::TSeparatorItem()
	:
	BSeparatorItem()
{
}


void
TSeparatorItem::Draw()
{
	BMenu* menu = Menu();
	if (menu == NULL)
		return;

	BRect frame(Frame());
	frame.right = frame.left + kSepItemWidth;
	rgb_color base = ui_color(B_MENU_BACKGROUND_COLOR);

	menu->PushState();

	menu->SetHighColor(tint_color(base, 1.22));
	frame.top--;
		// need to expand the frame for some reason

	// stroke a darker line on the left edge
	menu->StrokeLine(frame.LeftTop(), frame.LeftBottom());
	frame.left++;

	// fill in background
	be_control_look->DrawButtonBackground(menu, frame, frame, base);

	menu->PopState();
}


//	#pragma mark - TBarMenuBar


TBarMenuBar::TBarMenuBar(BRect frame, const char* name, TBarView* barView)
	:
	BMenuBar(frame, name, B_FOLLOW_NONE, B_ITEMS_IN_ROW, false),
	fBarView(barView),
	fAppListMenuItem(NULL),
	fSeparatorItem(NULL),
	fTeamIconData(NULL),
	fTeamIconSize(0)
{
	SetItemMargins(0.0f, 0.0f, 0.0f, 0.0f);
	SetFont(be_bold_font);

	TDeskbarMenu* beMenu = new TDeskbarMenu(barView);
	TBarWindow::SetDeskbarMenu(beMenu);

	float width = std::max(63.f, ceilf(63 * be_bold_font->Size() / 12.f));
	float height = std::max(22.f, ceilf(22 * be_bold_font->Size() / 12.f));
	BBitmap* icon = CreateSchwarzpauseMenuIcon(width * 2.0f, height * 2.0f);

	fDeskbarMenuItem = new TBarMenuTitle(0.0f, 0.0f, icon, beMenu, fBarView);
	AddItem(fDeskbarMenuItem);
}


TBarMenuBar::~TBarMenuBar()
{
}


void
TBarMenuBar::SmartResize(float width, float height)
{
	if (width == -1.0f && height == -1.0f) {
		BRect frame = Frame();
		width = frame.Width();
		height = frame.Height();
	} else
		ResizeTo(width, height);

	if (fSeparatorItem != NULL)
		fDeskbarMenuItem->SetContentSize(width - kSepItemWidth, height);
	else {
		int32 count = CountItems();
		if (fDeskbarMenuItem != NULL)
			fDeskbarMenuItem->SetContentSize(floorf(width / count), height);
		if (fAppListMenuItem != NULL)
			fAppListMenuItem->SetContentSize(floorf(width / count), height);
	}

	InvalidateLayout();
}


bool
TBarMenuBar::AddTeamMenu()
{
	if (CountItems() > 1)
		return false;

	BRect frame(Frame());

	delete fAppListMenuItem;
	fAppListMenuItem = new TBarMenuTitle(0.0f, 0.0f, FetchTeamIcon(),
		new TTeamMenu(fBarView), fBarView);

	bool added = AddItem(fAppListMenuItem, fBarView->Left() ? 0 : 1);

	if (added)
		SmartResize(frame.Width() - 1.0f, frame.Height());
	else
		SmartResize(frame.Width(), frame.Height());

	return added;
}


bool
TBarMenuBar::RemoveTeamMenu()
{
	if (CountItems() < 2)
		return false;

	bool removed = false;

	if (fAppListMenuItem != NULL && RemoveItem(fAppListMenuItem)) {
		delete fAppListMenuItem;
		fAppListMenuItem = NULL;
		SmartResize(-1, -1);
		removed = true;
	}

	return removed;
}


bool
TBarMenuBar::AddSeparatorItem()
{
	if (CountItems() > 1)
		return false;

	BRect frame(Frame());

	delete fSeparatorItem;
	fSeparatorItem = new TSeparatorItem();

	bool added = AddItem(fSeparatorItem);

	if (added)
		SmartResize(frame.Width() - 1.0f, frame.Height());
	else
		SmartResize(frame.Width(), frame.Height());

	return added;
}


bool
TBarMenuBar::RemoveSeperatorItem()
{
	if (CountItems() < 2)
		return false;

	bool removed = false;

	if (fSeparatorItem != NULL && RemoveItem(fSeparatorItem)) {
		delete fSeparatorItem;
		fSeparatorItem = NULL;
		SmartResize(-1, -1);
		removed = true;
	}

	return removed;
}


void
TBarMenuBar::Draw(BRect updateRect)
{
	// skip the fancy BMenuBar drawing code
	BMenu::Draw(updateRect);
}


void
TBarMenuBar::DrawBackground(BRect updateRect)
{
	BMenu::DrawBackground(updateRect);
}


void
TBarMenuBar::MouseMoved(BPoint where, uint32 code, const BMessage* message)
{
	// the following code parallels that in ExpandoMenuBar for DnD tracking

	if (!message) {
		// force a cleanup
		fBarView->DragStop(true);
		BMenuBar::MouseMoved(where, code, message);
		return;
	}

	switch (code) {
		case B_ENTERED_VIEW:
		{
			BPoint loc;
			uint32 buttons;
			GetMouse(&loc, &buttons);
			if (message != NULL && buttons != 0) {
				// attempt to start DnD tracking
				fBarView->CacheDragData(const_cast<BMessage*>(message));
				MouseDown(loc);
			}
			break;
		}
	}

	BMenuBar::MouseMoved(where, code, message);
}


static void
init_tracking_hook(BMenuItem* item, bool (*hookFunction)(BMenu*, void*),
	void* state)
{
	if (!item)
		return;

	BMenu* windowMenu = item->Submenu();
	if (windowMenu) {
		// have a menu, set the tracking hook
		windowMenu->SetTrackingHook(hookFunction, state);
	}
}


void
TBarMenuBar::InitTrackingHook(bool (*hookFunction)(BMenu*, void*),
	void* state, bool both)
{
	BPoint loc;
	uint32 buttons;
	GetMouse(&loc, &buttons);
	// set the hook functions for the two menus
	// will always have the deskbar menu
	// may have the app menu as well (mini mode)
	if (fDeskbarMenuItem->Frame().Contains(loc) || both)
		init_tracking_hook(fDeskbarMenuItem, hookFunction, state);

	if (fAppListMenuItem && (fAppListMenuItem->Frame().Contains(loc) || both))
		init_tracking_hook(fAppListMenuItem, hookFunction, state);
}


const BBitmap*
TBarMenuBar::FetchTeamIcon()
{
	// The team-menu icon is the Schwarzpause folders/files "organizer" glyph,
	// shipped as a white PNG (schwarzpause_team.png in icons.rdef) so it reads on
	// the dark Deskbar. Decode it and scale to the team-icon height, preserving
	// aspect ratio (the same PNG path the Start mark uses).
	float iconHeight = std::max(kTeamIconBitmapHeight,
		ceilf(kTeamIconBitmapHeight * be_bold_font->Size() / 12.f));
	// Supersample: render at 3x the display height, scaling by HEIGHT so a wide
	// icon keeps full width resolution. BarMenuTitle then bilinear-scales it down
	// to the (wider-than-tall) Deskbar slot crisply.
	float renderHeight = 3.0f * iconHeight;

	BBitmap* source = BTranslationUtils::GetBitmap(B_PNG_FORMAT,
		"schwarzpause_team.png");
	if (source == NULL)
		return NULL;

	float srcWidth = source->Bounds().Width() + 1;
	float srcHeight = source->Bounds().Height() + 1;
	float scale = renderHeight / srcHeight;
	float width = std::max(1.0f, floorf(srcWidth * scale));
	float height = std::max(1.0f, floorf(srcHeight * scale));

	BBitmap* icon = new(std::nothrow) BBitmap(BRect(0, 0, width - 1, height - 1),
		B_RGBA32, true);
	if (icon == NULL || icon->InitCheck() != B_OK) {
		delete source;
		delete icon;
		return NULL;
	}
	if (icon->Bits() != NULL && icon->BitsLength() > 0)
		memset(icon->Bits(), 0, icon->BitsLength());

	if (!icon->Lock()) {
		delete source;
		delete icon;
		return NULL;
	}
	BView* canvas = new(std::nothrow) BView(icon->Bounds(),
		"schwarzpause team icon", B_FOLLOW_NONE, B_WILL_DRAW);
	if (canvas == NULL) {
		icon->Unlock();
		delete source;
		delete icon;
		return NULL;
	}
	icon->AddChild(canvas);
	canvas->SetDrawingMode(B_OP_ALPHA);
	canvas->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_COMPOSITE);
	canvas->DrawBitmap(source, source->Bounds(), icon->Bounds(),
		B_FILTER_BITMAP_BILINEAR);
	canvas->Sync();
	icon->RemoveChild(canvas);
	icon->Unlock();
	delete canvas;
	delete source;

	return icon;
}
