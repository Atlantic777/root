// @(#)root/gl:$Name:  $:$Id: TGLDrawable.cxx,v 1.9 2005/11/22 18:05:46 brun Exp $
// Author:  Richard Maunder  25/05/2005

/*************************************************************************
 * Copyright (C) 1995-2000, Rene Brun and Fons Rademakers.               *
 * All rights reserved.                                                  *
 *                                                                       *
 * For the licensing terms see $ROOTSYS/LICENSE.                         *
 * For the list of contributors see $ROOTSYS/README/CREDITS.             *
 *************************************************************************/

#include "TGLDrawable.h"
#include "TGLDisplayListCache.h"

// For debug tracing
#include "TClass.h" 
#include "TError.h"

// For debug tracing
#include "TClass.h" 
#include "TError.h"

//////////////////////////////////////////////////////////////////////////
//                                                                      //
// TGLDrawable                                                          //      
//                                                                      //
// Abstract base class for all GL drawable objects - TGLPhysicalShape & //
// TGLLogicalShape hierarchy. Provides hooks for using the display list //
// cache in TGLDrawable::Draw() - the external draw method for all      //
// shapes.                                                              //
//                                                                      //
// Defines pure virtual TGLDrawable::DirectDraw() which derived classes //
// must implement with actual GL drawing.                               //
//                                                                      //
// Display list caching can occur at either the physical or logical     //
// level (with or without translation). Currently we cache only certain //
// derived logical shapes as not all logicals can respect the LOD draw  //
// flag which is used in caching.                                       //
//////////////////////////////////////////////////////////////////////////

ClassImp(TGLDrawable)

//______________________________________________________________________________
TGLDrawable::TGLDrawable(ULong_t ID, Bool_t DLCache) :
   fID(ID), fDLCache(DLCache)
{
   // Construct GL drawable object, with 'ID'. Bool 'DLCache'
   // indicates if draws should be captured to the display list cache
   // singleton (TGLDisplayListCache).
}

//______________________________________________________________________________
TGLDrawable::~TGLDrawable()
{
   // Destroy the GL drawable.
   Purge();
}

//______________________________________________________________________________
Bool_t TGLDrawable::UseDLCache(UInt_t /*LOD*/) const
{
   // Returns kTRUE if draw should used display list cache
   // KFALSE otherwise
   return fDLCache;
}

//______________________________________________________________________________
Bool_t TGLDrawable::SetDLCache(Bool_t DLCache)
{
   // Modify capture of draws into display list cache.
   // kTRUE - capture, kFALSE direct draw.
   if (DLCache == fDLCache) {
      return kFALSE;
   }

   fDLCache = DLCache;

   // Purge out any existing DL cache entries
   // Note: This does nothing at present as per drawable purging is not implemented
   // in TGLDisplayListCache.
   if (!fDLCache) {
      TGLDisplayListCache::Instance().Purge(*this);
   }

   return true;
}

//______________________________________________________________________________
void TGLDrawable::Draw(UInt_t LOD) const
{
   // Draw the GL drawable, using LOD draw flags. If DL caching is enabled
   // (see SetDLCache) then attempt to draw from the cache, if not found
   // attempt to capture the draw - done by DirectDraw() - into a new cache entry.
   // If not cached just call DirectDraw() for normal non DL cached drawing.
   
   // Debug tracing
   if (gDebug > 4) {
      Info("TGLDrawable::Draw", "this %d (class %s) LOD %d", this, IsA()->GetName(), LOD);
   }

   TGLDisplayListCache & cache = TGLDisplayListCache::Instance();

   // If shape is not cached, or a capture to cache is already in progress
   // perform a direct draw
   // DL can be nested, but not created in nested fashion. As we only
   // build DL on draw demands have to protected against this here.
   if (!UseDLCache(LOD) || cache.CaptureIsOpen())
   {
      DirectDraw(LOD);
      return;
   }

   // Attempt to draw from the cache
   if (!cache.Draw(*this, LOD))
   {
      // Capture the shape draw into compiled DL
      // If the cache is disabled the capture is ignored and
      // the shape is directly drawn
      cache.OpenCapture(*this, LOD);
      DirectDraw(LOD);

      // If capture was done then DL was just GL_COMPILE - we need to actually
      // draw it now
      if (cache.CloseCapture()) {
         Bool_t ok = cache.Draw(*this, LOD);
         assert(ok);
      }
   }
}

//______________________________________________________________________________
void TGLDrawable::Purge()
{
   // Purge all entries for all LODs for this drawable from the display list cache.
   //
   // Note: This does nothing at present as per drawable purging is not implemented
   // in TGLDisplayListCache.
   TGLDisplayListCache::Instance().Purge(*this);
}
