/* AbiWord
 * Copyright (C) 1998 AbiSource, Inc.
 * Copyright (C) 2002 Patrick Lam <plam@mit.edu>
 * Copyright (C) 2003 Martin Sevior <msevior@physics.unimelb.edu.au>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "fp_FrameContainer.h"
#include "fp_Column.h"
#include "fp_Page.h"
#include "fp_Line.h"
#include "fv_View.h"
#include "fl_DocLayout.h"
#include "fl_SectionLayout.h"
#include "gr_DrawArgs.h"
#include "ut_vector.h"
#include "ut_types.h"
#include "ut_debugmsg.h"
#include "ut_assert.h"
#include "fl_FrameLayout.h"
#include "fp_TableContainer.h"

/*!
  Create Frame container
  \param iType Container type
  \param pSectionLayout Section layout type used for this container
 */
fp_FrameContainer::fp_FrameContainer(fl_SectionLayout* pSectionLayout) 
	: fp_VerticalContainer(FP_CONTAINER_FRAME, pSectionLayout),
	  m_pPage(NULL),
	  m_iXpad(0),
	  m_iYpad(0),
	  m_bNeverDrawn(true)
{
}

/*!
  Destruct container
  \note The Containers in vector of the container are not
        destructed. They are owned by the logical hierarchy (i.e.,
		the fl_Container classes like fl_BlockLayout), not the physical
        hierarchy.
 */
fp_FrameContainer::~fp_FrameContainer()
{
	m_pPage = NULL;
}

void fp_FrameContainer::setPage(fp_Page * pPage)
{
	if(pPage && (m_pPage != NULL) && m_pPage != pPage)
	{
		clearScreen();
		m_pPage->removeFrameContainer(this);
		getSectionLayout()->markAllRunsDirty();
	}
	m_pPage = pPage;
	if(pPage)
	{
		getFillType()->setParent(pPage->getFillType());
	}
	else
	{
		getFillType()->setParent(NULL);
	}
}

void fp_FrameContainer::clearScreen(void)
{
	fp_Page * pPage = getPage();
	if(pPage == NULL)
	{
		return;
	}
	if(getView() == NULL)
	{
		return;
	}
	UT_sint32 srcX,srcY;
	UT_sint32 xoff,yoff;
	getView()->getPageScreenOffsets(pPage,xoff,yoff);
	xxx_UT_DEBUGMSG(("pagescreenoffsets xoff %d yoff %d \n",xoff,yoff));
	UT_sint32 leftThick = m_lineLeft.m_thickness;
	UT_sint32 rightThick = m_lineRight.m_thickness;
	UT_sint32 topThick = m_lineTop.m_thickness;
	UT_sint32 botThick = m_lineBottom.m_thickness;

	srcX = getFullX() - leftThick;
	srcY = getFullY() - topThick;

	xoff += getFullX() - leftThick;
	yoff += getFullY() - topThick;
	getFillType()->getParent()->Fill(getGraphics(),srcX,srcY,xoff,yoff,getFullWidth()+leftThick+rightThick,getFullHeight()+topThick+botThick+getGraphics()->tlu(1) +1);
	fp_Container * pCon = NULL;
	UT_sint32 i = 0;
	for(i=0; i< static_cast<UT_sint32>(countCons()); i++)
	{
		pCon = static_cast<fp_Container *>(getNthCon(i));
		pCon->clearScreen();
	}
}

/*!
 * All these methods are used to implement an X and Y padding around the
 * Frame
 */
UT_sint32 fp_FrameContainer::getFullWidth(void) const
{
	return fp_VerticalContainer::getWidth();
}

UT_sint32 fp_FrameContainer::getFullHeight(void) const
{
	return fp_VerticalContainer::getHeight();
}

UT_sint32 fp_FrameContainer::getFullX(void) const
{
	return fp_VerticalContainer::getX();
}

UT_sint32 fp_FrameContainer::getFullY(void) const
{
	return fp_VerticalContainer::getY();
}


UT_sint32 fp_FrameContainer::getWidth(void) const
{
	UT_sint32 iWidth = fp_VerticalContainer::getWidth() - m_iXpad*2;
	return iWidth;
}

UT_sint32 fp_FrameContainer::getX(void) const
{
	UT_sint32 iX = fp_VerticalContainer::getX() + m_iXpad;
	return iX;
}


UT_sint32 fp_FrameContainer::getY(void) const
{
	UT_sint32 iY = fp_VerticalContainer::getY() + m_iYpad;
	return iY;
}

UT_sint32 fp_FrameContainer::getHeight(void) const
{
	UT_sint32 iHeight = fp_VerticalContainer::getHeight() - m_iYpad*2;
	return iHeight;
}

	
void fp_FrameContainer::setContainer(fp_Container * pContainer)
{
	UT_ASSERT(UT_SHOULD_NOT_HAPPEN);
}

fl_DocSectionLayout * fp_FrameContainer::getDocSectionLayout(void)
{
	fl_FrameLayout * pFL = static_cast<fl_FrameLayout *>(getSectionLayout());
	fl_DocSectionLayout * pDSL = static_cast<fl_DocSectionLayout *>(pFL->myContainingLayout());
	UT_ASSERT(pDSL && (pDSL->getContainerType() == FL_CONTAINER_DOCSECTION));
	return pDSL;
}


/* just a little helper function
 */
void fp_FrameContainer::_drawLine (const PP_PropertyMap::Line & style,
								  UT_sint32 left, UT_sint32 top, UT_sint32 right, UT_sint32 bot,GR_Graphics * pGr)
{

	if (style.m_t_linestyle == PP_PropertyMap::linestyle_none)
		return; // do not draw	
	
	GR_Graphics::JoinStyle js = GR_Graphics::JOIN_MITER;
	GR_Graphics::CapStyle  cs = GR_Graphics::CAP_BUTT;

	switch (style.m_t_linestyle)
	{
		case PP_PropertyMap::linestyle_dotted:
			pGr->setLineProperties (1, js, cs, GR_Graphics::LINE_DOTTED);
			break;
		case PP_PropertyMap::linestyle_dashed:
			pGr->setLineProperties (1, js, cs, GR_Graphics::LINE_ON_OFF_DASH);
			break;
		case PP_PropertyMap::linestyle_solid:
			pGr->setLineProperties (1, js, cs, GR_Graphics::LINE_SOLID);
			break;
		default: // do nothing; shouldn't happen
			break;
	}

	pGr->setLineWidth (static_cast<UT_sint32>(style.m_thickness));
	pGr->setColor (style.m_color);


	xxx_UT_DEBUGMSG(("_drawLine: top %d bot %d \n",top,bot));

	pGr->drawLine (left, top, right, bot);
	
	pGr->setLineProperties (1, js, cs, GR_Graphics::LINE_SOLID);
}

/*!
 * Draw the frame boundaries
 */
void  fp_FrameContainer::drawBoundaries(dg_DrawArgs * pDA)
{
	UT_sint32 iXlow = pDA->xoff - m_iXpad;
	UT_sint32 iXhigh = iXlow + getFullWidth() ;
	UT_sint32 iYlow = pDA->yoff - m_iYpad;
	UT_sint32 iYhigh = iYlow + getFullHeight();
	_drawLine(m_lineTop,iXlow,iYlow,iXhigh,iYlow,pDA->pG); // top
	_drawLine(m_lineRight,iXhigh,iYlow,iXhigh,iYhigh,pDA->pG); // right
	_drawLine(m_lineBottom,iXlow,iYhigh,iXhigh,iYhigh,pDA->pG); // bottom
	_drawLine(m_lineLeft,iXlow,iYlow,iXlow,iYhigh,pDA->pG); // left
}


/*!
 * Draw the frame handles
 */
void  fp_FrameContainer::drawHandles(dg_DrawArgs * pDA)
{
	UT_sint32 iXlow = pDA->xoff - m_iXpad;
	UT_sint32 iXhigh = iXlow + getFullWidth() ;
	UT_sint32 iYlow = pDA->yoff - m_iYpad;
	UT_sint32 iYhigh = iYlow + getFullHeight();
	UT_sint32 iXMid = (iXlow + iXhigh)/2;
	UT_sint32 iYMid = (iYlow + iYhigh)/2;
	GR_Graphics * pG = pDA->pG;
	UT_sint32 res = pG->tlu(8);
	_drawHandleBox(UT_Rect(iXlow,iYlow,res,res));
	_drawHandleBox(UT_Rect(iXMid-res/2,iYlow,res,res));
	_drawHandleBox(UT_Rect(iXhigh-res,iYlow,res,res));
	_drawHandleBox(UT_Rect(iXlow,iYMid-res/2,res,res));
	_drawHandleBox(UT_Rect(iXhigh-res,iYMid-res/2,res,res));
	_drawHandleBox(UT_Rect(iXlow,iYhigh-res,res,res));
	_drawHandleBox(UT_Rect(iXMid-res/2,iYhigh-res,res,res));
	_drawHandleBox(UT_Rect(iXhigh-res,iYhigh-res,res,res));
}

void fp_FrameContainer::_drawHandleBox(UT_Rect box)
{
//
// Code cut and pasted from uwog's handle boxes on images.
//
	UT_sint32 left = box.left;
	UT_sint32 top = box.top;
	UT_sint32 right = box.left + box.width - getGraphics()->tlu(1);
	UT_sint32 bottom = box.top + box.height - getGraphics()->tlu(1);
	
	getGraphics()->setLineProperties(1.0,
								 GR_Graphics::JOIN_MITER,
								 GR_Graphics::CAP_BUTT,
								 GR_Graphics::LINE_SOLID);	
	
	// draw some really fancy box here
	getGraphics()->setColor(UT_RGBColor(98,129,131));
	getGraphics()->drawLine(left, top, right, top);
	getGraphics()->drawLine(left, top, left, bottom);
	getGraphics()->setColor(UT_RGBColor(230,234,238));
	getGraphics()->drawLine(box.left+getGraphics()->tlu(1), box.top + getGraphics()->tlu(1), right - getGraphics()->tlu(1), top+getGraphics()->tlu(1));
	getGraphics()->drawLine(box.left+getGraphics()->tlu(1), box.top + getGraphics()->tlu(1), left + getGraphics()->tlu(1), bottom - getGraphics()->tlu(1));
	getGraphics()->setColor(UT_RGBColor(98,129,131));
	getGraphics()->drawLine(right - getGraphics()->tlu(1), top + getGraphics()->tlu(1), right - getGraphics()->tlu(1), bottom - getGraphics()->tlu(1));
	getGraphics()->drawLine(left + getGraphics()->tlu(1), bottom - getGraphics()->tlu(1), right - getGraphics()->tlu(1), bottom - getGraphics()->tlu(1));
	getGraphics()->setColor(UT_RGBColor(49,85,82));
	getGraphics()->drawLine(right, top, right, bottom);
	getGraphics()->drawLine(left, bottom, right, bottom);
	getGraphics()->fillRect(UT_RGBColor(156,178,180),box.left + getGraphics()->tlu(2), box.top + getGraphics()->tlu(2), box.width - getGraphics()->tlu(4), box.height - getGraphics()->tlu(4));

}

/*!
 Draw container content
 \param pDA Draw arguments
 */
void fp_FrameContainer::draw(dg_DrawArgs* pDA)
{
	if(getPage() == NULL)
	{
		return;
	}
	if(getView())
	{
		if(getView()->getFrameEdit()->getFrameEditMode() == FV_FrameEdit_DRAG_EXISTING)
		{
			if((getView()->getFrameEdit()->getFrameContainer() == this))
			{
				return;
			}
		}
	}
//
// Only draw the lines in the clipping region.
//
	dg_DrawArgs da = *pDA;
	GR_Graphics * pG = da.pG;
	if(!pDA->bDirtyRunsOnly || m_bNeverDrawn)
	{
		if(m_bNeverDrawn)
		{
			pDA->bDirtyRunsOnly= false;
		} 
		UT_sint32 srcX,srcY;

		srcX = -m_iXpad;
		srcY = -m_iYpad;

		UT_sint32 x = pDA->xoff - m_iXpad;
		UT_sint32 y = pDA->yoff - m_iYpad;
		getFillType()->Fill(pG,srcX,srcY,x,y,getFullWidth(),getFullHeight());
	}
	UT_uint32 count = countCons();
	for (UT_uint32 i = 0; i<count; i++)
	{
		fp_ContainerObject* pContainer = static_cast<fp_ContainerObject*>(getNthCon(i));
		da.xoff = pDA->xoff + pContainer->getX();
		da.yoff = pDA->yoff + pContainer->getY();
		pContainer->draw(&da);
	}
	m_bNeverDrawn = false;
	drawBoundaries(pDA);
}

void fp_FrameContainer::setBackground (const PP_PropertyMap::Background & style)
{
	m_background = style;
	PP_PropertyMap::Background background = m_background;
	if(background.m_t_background == PP_PropertyMap::background_solid)
	{
		getFillType()->setColor(background.m_color);
	}
}


/*!
 * FrameContainers are not in the linked list of physical containers
 */
fp_Container * fp_FrameContainer::getNextContainerInSection() const
{
	return NULL;
}

/*!
 * FrameContainers are not in the linked list of physical containers
 */
fp_Container * fp_FrameContainer::getPrevContainerInSection() const
{
	return NULL;
}

void fp_FrameContainer::layout(void)
{
	_setMaxContainerHeight(0);
	UT_sint32 iY = 0, iPrevY = 0;
	iY= 0;
	UT_uint32 iCountContainers = countCons();
	fp_Container *pContainer, *pPrevContainer = NULL;
	for (UT_uint32 i=0; i < iCountContainers; i++)
	{
		pContainer = static_cast<fp_Container*>(getNthCon(i));
//
// This is to speedup redraws.
//
		if(pContainer->getHeight() > _getMaxContainerHeight())
			_setMaxContainerHeight(pContainer->getHeight());

		if(pContainer->getY() != iY)
		{
			pContainer->clearScreen();
		}
		if(iY > getHeight())
		{
			pContainer->setY(-1000000);
		}
		else
		{
			pContainer->setY(iY);
		}
		UT_sint32 iContainerHeight = pContainer->getHeight();
		UT_sint32 iContainerMarginAfter = pContainer->getMarginAfter();
		if(pContainer->getContainerType() == FP_CONTAINER_TABLE)
		{
			fp_TableContainer * pTab = static_cast<fp_TableContainer *>(pContainer);
			iContainerHeight = pTab->getHeight();
			if(!pTab->isThisBroken() && (pTab->getFirstBrokenTable() == NULL))
			{
				fp_Container * pBroke = static_cast<fp_Container *>(pTab->VBreakAt(0));
			}
		}

		iY += iContainerHeight;
		iY += iContainerMarginAfter;
		//iY +=  0.5;

		if (pPrevContainer)
		{
			pPrevContainer->setAssignedScreenHeight(iY - iPrevY);
		}
		pPrevContainer = pContainer;
		iPrevY = iY;
	}

	// Correct height position of the last line
	if (pPrevContainer)
	{
		if(iY > getHeight())
		{
			pPrevContainer->setAssignedScreenHeight(-1000000);
		}
		else
		{
			pPrevContainer->setAssignedScreenHeight(iY - iPrevY + 1);
		}
	}
}
