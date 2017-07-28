
// MFCPlayer.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// CMFCPlayerApp:
// See MFCPlayer.cpp for the implementation of this class
//

class CMFCPlayerApp : public CWinApp
{
public:
	CMFCPlayerApp();

// Overrides
public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern CMFCPlayerApp theApp;