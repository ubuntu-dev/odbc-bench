/*
 *  main.cpp
 *
 *  $Id$
 *
 *  odbc-bench - a TPC-A and TPC-C like benchmark program for databases
 *  Copyright (C) 2000-2005 OpenLink Software <odbc-bench@openlinksw.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <memory>

#include "odbcbench_macosx.h"
#include "util.h"
#include "SplitView.h"
#include "TestPoolItemList.h"
#include "LoginDialog.h"
#include "StatusDialog.h"

#include "tpca_code.h"
#include "tpccfun.h"
#include "results.h"

// main nib
IBNibRef g_main_nib;

// File menu
#define CMD_CLEAR_LOG				3000

// Edit menu
#define CMD_NEW_ITEM				4000
#define CMD_DELETE_ITEMS			4001
#define CMD_SAVE_ITEMS				4002
#define CMD_LOGIN_DETAILS			4003
#define CMD_TABLE_DETAILS			4004
#define CMD_RUN_DETAILS				4005
#define CMD_INSERT_FILE				4006

// Actions menu
#define CMD_CREATE_TABLES			5000
#define CMD_DROP_TABLES				5001
#define CMD_RUN						5002

// Results menu
#define CMD_RESULTS_CONNECT			6000
#define CMD_RESULTS_DISCONNECT		6001
#define CMD_RESULTS_CREATE_TABLE	6002
#define CMD_RESULTS_DROP_TABLE		6003

static ControlID kMainSplitViewID = { 'MAIN', 0 };
static ControlID kMainItemViewID = { 'MAIN', 1 };
static ControlID kMainLogViewID = { 'MAIN', 2 };

OSStatus
OPL_ItemViewItemDataCallback(ControlRef itemView, DataBrowserItemID itemID, 
	DataBrowserPropertyID property, DataBrowserItemDataRef itemData, 
	Boolean changeValue)
{  
    OSStatus err;
	WindowRef window = GetControlOwner(itemView);
	test_t *test;
	CFStringRef str_release = NULL;
	
	if (changeValue)
		return errDataBrowserPropertyNotSupported;

	// Get TestPool instance
	OPL_TestPool *testPool;
	err = GetWindowProperty(window, kPropertyCreator, kPropertyTag,
	   sizeof(testPool), NULL, &testPool);
	require_noerr(err, error);

	// Get test item
	test = testPool->getItem(itemID);
	CFStringRef str;
	
    switch (property) {
    case kItemViewType:
		str = (test->TestType == TPC_A) ? CFSTR("TPC-A") : CFSTR("TPC-C");
		err = SetDataBrowserItemDataText(itemData, str);
		break;

	case kItemViewName:
		str_release = str = OPL_char_to_CFString(test->szName);
		err = SetDataBrowserItemDataText(itemData, str);
		break;
		
	case kItemViewDSN:
		str_release = str = OPL_char_to_CFString(test->szLoginDSN);
		err = SetDataBrowserItemDataText(itemData, str);
		break;
		
	case kItemViewDriverName:
		str_release = str = OPL_char_to_CFString(test->szDriverName);
		err = SetDataBrowserItemDataText(itemData, str);
		break;
		
	case kItemViewDriverVer:
		str_release = str = OPL_char_to_CFString(test->szDriverVer);
		err = SetDataBrowserItemDataText(itemData, str);
		break;
		
	case kItemViewDBMS:
		str_release = str = OPL_char_to_CFString(test->szDBMS);
		err = SetDataBrowserItemDataText(itemData, str);
		break;
		
	default:
		return errDataBrowserPropertyNotSupported;
    }

#if 0
	UInt16 width, newWidth;
	err = GetDataBrowserTableViewNamedColumnWidth(itemView, property, &width);
	require_noerr(err, error);
	
	// this is crap
	newWidth = CharWidth('W') * CFStringGetLength(str);
	if (newWidth > width) {
		err = SetDataBrowserTableViewNamedColumnWidth(
			itemView, property, newWidth);
		require_noerr(err, error);
		
		// auto-size test item browser columns
		err = AutoSizeDataBrowserListViewColumns(itemView);
		require_noerr(err, error);
	}
#endif
	
error:
	if (str_release != NULL)
		CFRelease(str_release);
	return err;
}

OSStatus
OPL_LogViewItemDataCallback(ControlRef logView, DataBrowserItemID itemID, 
	DataBrowserPropertyID property, DataBrowserItemDataRef itemData, 
	Boolean changeValue)
{  
    OSStatus err;
	WindowRef window = GetControlOwner(logView);

	if (changeValue)
		return errDataBrowserPropertyNotSupported;

	// Get TestPool instance
	OPL_TestPool *testPool;
	err = GetWindowProperty(window, kPropertyCreator, kPropertyTag,
	   sizeof(testPool), NULL, &testPool);
	require_noerr(err, error);

    switch (property) {
    case kLogViewLog:
		err = SetDataBrowserItemDataText(itemData, testPool->getLogMsg(itemID));
		break;
		
	default:
		return errDataBrowserPropertyNotSupported;
    }

error:
	return err;
}

// Create new main window
OSStatus
OPL_NewWindow(CFStringRef filename /* = NULL */)
{
	OSStatus err;
	WindowRef window;
	ControlRef splitView;
	HIRect hiBounds;
	float ratio = 0.66;
	HILayoutInfo layoutInfo;
	HIViewRef parentView;
	ControlRef itemView;
	ControlRef logView;
	OPL_TestPool *testPool;
	
    // create main window
    err = CreateWindowFromNib(g_main_nib, CFSTR("MainWindow"), &window);
	require_noerr(err, error);

	// find test item browser
	err = GetControlByID(window, &kMainItemViewID, &itemView);
	require_noerr(err, error);

	// find log browser
	err = GetControlByID(window, &kMainLogViewID, &logView);
	require_noerr(err, error);
	
	// create instance data and associate it with the window
	testPool = new OPL_TestPool(itemView, logView, filename);
	err = SetWindowProperty(window, kPropertyCreator, kPropertyTag,
		sizeof(testPool), &testPool);
	require_noerr(err, error);

	// base the item/log view size on the start window size
	err = HIViewFindByID(HIViewGetRoot(window), kHIViewWindowContentID, &parentView);
	require_noerr(err, error);
	err = HIViewGetFrame(parentView, &hiBounds);
	require_noerr(err, error);
	hiBounds.size.width -= hiBounds.origin.x;
	hiBounds.size.height -= hiBounds.origin.y;
	hiBounds.origin.x = hiBounds.origin.y = 0;
	
	// make the main split view
	err = SplitView::Create(&splitView, &hiBounds, window);
	require_noerr(err, error);
	
	// set initial split ratio
	err = SetControlData(splitView, 0, kControlSplitViewSplitRatioTag, sizeof(float), &ratio);
	require_noerr(err, error);
	
	ShowControl(splitView);
		
	// give it an ID, so we can find it later
	SetControlID(splitView, &kMainSplitViewID);

	// make the main split view resize along with main window
	layoutInfo.version = kHILayoutInfoVersionZero;
	err = HIViewGetLayoutInfo(splitView, &layoutInfo);
	require_noerr(err, error);
	layoutInfo.binding.top.toView = parentView;
	layoutInfo.binding.top.kind = kHILayoutBindTop;
	layoutInfo.binding.left.toView = parentView;
	layoutInfo.binding.left.kind = kHILayoutBindLeft;
	layoutInfo.binding.right.toView = parentView;
	layoutInfo.binding.right.kind = kHILayoutBindRight;
	layoutInfo.binding.bottom.toView = parentView;
	layoutInfo.binding.bottom.kind = kHILayoutBindBottom;
	err = HIViewSetLayoutInfo(splitView, &layoutInfo);
	require_noerr(err, error);
	
	// add it as A view of the main split view
	err = SetControlData(splitView, 0, kControlSplitViewSubViewA,
		sizeof(itemView), &itemView);
	require_noerr(err, error);
	
	// set item browser callbacks
	DataBrowserCallbacks itemViewCallbacks;
	itemViewCallbacks.version = kDataBrowserLatestCallbacks;
    err = InitDataBrowserCallbacks(&itemViewCallbacks);
	require_noerr(err, error);
	
	static DataBrowserItemDataUPP g_itemItemDataUPP = NULL;
	if (!g_itemItemDataUPP) {
		g_itemItemDataUPP = NewDataBrowserItemDataUPP(
			OPL_ItemViewItemDataCallback);
	}
	
    itemViewCallbacks.u.v1.itemDataCallback = g_itemItemDataUPP;
    err = SetDataBrowserCallbacks(itemView, &itemViewCallbacks); 
	require_noerr(err, error);
	
	// auto-size test item browser columns
	err = AutoSizeDataBrowserListViewColumns(itemView);
	require_noerr(err, error);
	
	// set log browser callbacks
	DataBrowserCallbacks logViewCallbacks;
	logViewCallbacks.version = kDataBrowserLatestCallbacks;
    err = InitDataBrowserCallbacks(&logViewCallbacks);
	require_noerr(err, error);
	
	static DataBrowserItemDataUPP g_logItemDataUPP = NULL;
	if (!g_logItemDataUPP) {
		g_logItemDataUPP = NewDataBrowserItemDataUPP(
			OPL_LogViewItemDataCallback);
	}
	
    logViewCallbacks.u.v1.itemDataCallback = g_logItemDataUPP;
    err = SetDataBrowserCallbacks(logView, &logViewCallbacks); 
	require_noerr(err, error);
	
	// add it as B view of the main split view
	err = SetControlData(splitView, 0, kControlSplitViewSubViewB,
		sizeof(logView), &logView);
	require_noerr(err, error);

    // the window was created hidden so show it.
    ShowWindow(window);
	
error:
	return err;
}

// Close window
OSStatus
OPL_CloseWindow()
{
	OPL_TestPool *testPool;

	testPool = OPL_TestPool::get();
	if (testPool != NULL) {
		EventRef event;
		
		WindowRef window = testPool->getWindow();
		delete testPool;
		RemoveWindowProperty(window, kPropertyCreator, kPropertyTag);
			
		CreateEvent(NULL, kEventClassWindow, kEventWindowClose, 0, 0, &event);
		SetEventParameter(event, kEventParamDirectObject, typeWindowRef,
			sizeof(window), &window);
		SendEventToEventTarget(event, GetWindowEventTarget(window));
	}
	
	return noErr;
}

// Process menu command
pascal OSStatus
app_event_handler(EventHandlerCallRef handlerRef, EventRef eventRef, void *userData)
{
	OSStatus err = eventNotHandledErr;
	UInt32 eventClass = GetEventClass(eventRef);
	UInt32 eventKind = GetEventKind(eventRef);
	HICommand cmd;

	if (eventClass != kEventClassCommand && eventKind != kEventCommandProcess)
		return err;
	
	// Obtain HICommand
	err = GetEventParameter(eventRef, kEventParamDirectObject,  
		typeHICommand, NULL, sizeof(cmd), NULL, &cmd);
	require_noerr(err, error);
	
	// Process commands
	err = noErr;
	switch (cmd.commandID) {
// Application menu	
	case kHICommandPreferences:
		do_preferences();
		break;

// File menu
	case kHICommandNew:
		err = OPL_NewWindow();
		break;

	case kHICommandOpen:
		do_file_open();
		break;
		
	case kHICommandSave:
		do_file_save();
		break;
		
	case kHICommandSaveAs:
		do_file_save_as();
		break;
		
	case kHICommandClose:
		err = OPL_CloseWindow();
		break;
		
	case CMD_CLEAR_LOG:
		do_clear_log();
		break;
		
// Edit menu
	case CMD_NEW_ITEM:
		do_new_item();
		break;
		
	case CMD_DELETE_ITEMS:
		do_delete_items();
		break;
		
	case CMD_SAVE_ITEMS:
		do_save_items();
		break;
		
	case CMD_LOGIN_DETAILS:
		do_login_details();
		break;
		
	case CMD_TABLE_DETAILS:
		do_table_details();
		break;
		
	case CMD_RUN_DETAILS:
		do_run_details();
		break;
		
	case CMD_INSERT_FILE:
		do_insert_file();
		break;

// Actions menu		
	case CMD_CREATE_TABLES:
		do_create_tables();
		break;
		
	case CMD_DROP_TABLES:
		do_drop_tables();
		break;
		
	case CMD_RUN:
		do_run();
		break;

// Results menu		
	case CMD_RESULTS_CONNECT:
		do_results_connect();
		break;
		
	case CMD_RESULTS_DISCONNECT:
		do_results_disconnect();
		break;
		
	case CMD_RESULTS_CREATE_TABLE:
		do_results_create_table();
		break;
		
	case CMD_RESULTS_DROP_TABLE:
		do_results_drop_table();
		break;
		
	default:
		return eventNotHandledErr;
	}
		
error:
	return err;
}

// odbc-bench callbacks
void
OPL_pane_log(const char *format, ...)
{
	CFStringRef str;
	OPL_TestPool *testPool;

	testPool = OPL_TestPool::get();
	if (testPool == NULL)
		return;
	
	// Create message
	va_list ap;
	va_start(ap, format);
	str = OPL_CFString_vasprintf(format, ap);
	va_end(ap);
	
	// Append a line
	testPool->addLogMsg(str);
}

void (*pane_log) (const char *format, ...) = OPL_pane_log;
GUI_t gui;
extern "C" int do_command_line (int argc, char *argv[]);

void
OPL_alert(AlertType alertType, ConstStr255Param inError,
	const char *fmt, va_list ap)
{
	Str255 msg;
	
	CFStringRef str = OPL_CFString_vasprintf(fmt, ap);
	CFStringGetPascalString(str, msg, sizeof(msg), kCFStringEncodingASCII);
	CFRelease(str);
	
	StandardAlert(alertType, inError, msg, NULL, NULL);
}

void
OPL_err_message(const char *fmt, ...)
{
	va_list ap;
	
	va_start(ap, fmt);
	OPL_alert(kAlertStopAlert, "\pError", fmt, ap);
	va_end(ap);
}

void
OPL_warn_message(const char *fmt, ...)
{
	va_list ap;
	
	va_start(ap, fmt);
	OPL_alert(kAlertCautionAlert, "\pWarning", fmt, ap);
	va_end(ap);
}

void
OPL_message(const char *fmt, ...)
{
	va_list ap;
	
	va_start(ap, fmt);
	OPL_alert(kAlertNoteAlert, "\pNote", fmt, ap);
	va_end(ap);
}

int
OPL_add_test_to_the_pool(test_t *test)
{
	OPL_TestPool *testPool;

	testPool = OPL_TestPool::get();
	if (testPool == NULL)
		return 0;
	
	// add an item
	testPool->addItem(test);
	return 1;
	
error:
	return 0;
}

OPL_StatusDialog *g_progress;
bool g_cancelled;

// Create and show progress window
void
OPL_ShowProgress(void *parent_win, char *title, BOOL forceSingle, float nMax)
{
	bool res;
	CFStringRef text;
	
	if (g_progress != NULL) {
		delete g_progress;
		g_progress = NULL;
	}
	
	if (forceSingle)
		g_progress = new OPL_SimpleStatusDialog();
	else
		g_progress = new OPL_TestRunStatusDialog();
	if (g_progress == NULL)
		return;

	text = OPL_char_to_CFString(title);
	res = g_progress->begin(text, nMax);
	CFRelease(text);
	
	if (!res) {
			delete g_progress;
			g_progress = NULL;
			return;
	}
}

// Destroy progress windows
void
OPL_StopProgress(void)
{
	if (g_progress == NULL)
		return;
	
	g_progress->end();
	delete g_progress;
	g_progress = NULL;
}

// Set working item
void
OPL_SetWorkingItem(char *working)
{
	if (g_progress == NULL)
		return;
		
	g_progress->setWorkItem(OPL_char_to_CFString(working));
}

// Set progress text
void
OPL_SetProgressText(char *progress, int nConn, int threadNo, 
	float nValue, int nTrnPerCall,
	long /* secsRemain */, double /* tpca_dDiffSum */)
{
	if (g_progress == NULL)
		return;

	g_progress->setProgressText(OPL_char_to_CFString(progress),
		nConn, threadNo, nValue, nTrnPerCall);
}

// Mark work item as finished
void
OPL_mark_finished(int nConn, int threadNo)
{
	if (g_progress == NULL)
		return;
		
	g_progress->markFinished(nConn, threadNo);
}

// Process events and test if the work item is cancelled
int
OPL_fCancel(void)
{
	if (g_progress == NULL)
		return g_cancelled;
		
	g_progress->runEventLoop();
	if (!g_progress->getStatus())
		g_cancelled = true;
		
	return g_cancelled;
}

// Test if work item is canceled
BOOL
OPL_isCancelled(void)
{
	return g_cancelled;
}

// Main function
int 
main(int argc, char *argv[])
{
    static EventTypeSpec menu_events[] = {
		{ kEventClassCommand, kEventCommandProcess }
	};
    OSStatus		err;

	do_alloc_env();
	
	// gui.main_quit = OPL_main_quit;				// NOT NEEDED
	gui.err_message = OPL_err_message;
	gui.warn_message = OPL_warn_message;
	gui.message = OPL_message;
	gui.add_test_to_the_pool = OPL_add_test_to_the_pool;
	// gui.for_all_in_pool = OPL_for_all_in_pool;	// NOT USED

	gui.do_MarkFinished = OPL_mark_finished;
	gui.isCancelled = OPL_isCancelled;
	gui.ShowProgress = OPL_ShowProgress;
	gui.SetWorkingItem = OPL_SetWorkingItem;
	gui.SetProgressText = OPL_SetProgressText;
	gui.StopProgress = OPL_StopProgress;
	gui.fCancel = OPL_fCancel;
	// gui.vBusy = OPL_vBusy;						// NOT NEEDED
	
	if (argc > 2)
		return do_command_line(argc, argv);
	
    // Create a Nib reference passing the name of the nib file (without the .nib extension)
    // CreateNibReference only searches into the application bundle.
    err = CreateNibReference(CFSTR("main"), &g_main_nib);
    require_noerr( err, CantGetNibRef );
    
    // Once the nib reference is created, set the menu bar. "MainMenu" is the name of the menu bar
    // object. This name is set in InterfaceBuilder when the nib is created.
    err = SetMenuBarFromNib(g_main_nib, CFSTR("MenuBar"));
    require_noerr(err, CantSetMenuBar);
    
    // Enable "Preferences..." menu
	EnableMenuCommand(NULL, kHICommandPreferences);
	
	// Install handler for menu commands
	err = InstallApplicationEventHandler(NewEventHandlerUPP(app_event_handler),
		GetEventTypeCount(menu_events), menu_events, NULL, NULL);
	require_noerr(err, CantInstallEventHandler);

	err = OPL_NewWindow();
    require_noerr(err, CantCreateWindow);
    
	// Call the event loop
    RunApplicationEventLoop();

CantCreateWindow:
CantSetMenuBar:
CantGetNibRef:
CantInstallEventHandler:
	return err;
}

// Application menu

static ControlID kPrefRefreshRate = { 'PREF', 0 };
static ControlID kPrefLockTimeout = { 'PREF', 1 };

void
do_preferences()
{
	OPL_Dialog dialog(CFSTR("PreferencesDialog"));
	
	// set initial values
	dialog.setEditText(kPrefRefreshRate, OPL_CFString_asprintf("%ld",
		bench_get_long_pref(DISPLAY_REFRESH_RATE)));
	dialog.setEditText(kPrefLockTimeout, OPL_CFString_asprintf("%ld",
		bench_get_long_pref(LOCK_TIMEOUT)));
	
	// run dialog
	if (!dialog.run())
		return;

	// fetch dialog data
	bench_set_long_pref(DISPLAY_REFRESH_RATE,
		dialog.getEditTextIntValue(kPrefRefreshRate));
	bench_set_long_pref(LOCK_TIMEOUT,
		dialog.getEditTextIntValue(kPrefLockTimeout));
}

// File menu

void
do_file_open()
{
	do_open(NULL);
}

void
do_file_save()
{
	std::auto_ptr<OPL_TestPoolItemList> allItems(
		OPL_TestPoolItemList::getAll());
	if (allItems.get() == NULL)
		return;

	do_save(allItems.get(), false, true);
}

void
do_file_save_as()
{
	std::auto_ptr<OPL_TestPoolItemList> allItems(
		OPL_TestPoolItemList::getAll());
	if (allItems.get() == NULL)
		return;

	do_save(allItems.get(), true, true);
}

void
do_clear_log()
{
	OPL_TestPool *testPool;

	testPool = OPL_TestPool::get();
	if (testPool == NULL)
		return;
	
	testPool->clearLog();
}

// Edit menu

static ControlID kEditNewItemName = { 'NEW ', 0 };
static ControlID kEditNewItemType = { 'NEW ', 1 };

void
do_new_item()
{
	OPL_Dialog dialog(CFSTR("EditNewBenchmarkDialog"));
	
	// run dialog
	if (!dialog.run())
		return;
	
	// fetch dialog data
	test_t *test = (test_t *) calloc (1, sizeof(*test));

	char *testName = OPL_CFString_to_char(
		dialog.getEditText(kEditNewItemName));
	strncpy(test->szName, testName, sizeof(test->szName) - 1);
	free(testName);
	test->TestType = dialog.get32BitValue(kEditNewItemType) == 1 ?
		TPC_A : TPC_C;
	test->is_dirty = TRUE;
	init_test(test);

	if (!OPL_add_test_to_the_pool(test))
		free(test);
}

void
do_delete_items()
{
	std::auto_ptr<OPL_TestPoolItemList> selectedItems(
		OPL_TestPoolItemList::getSelected());
	if (selectedItems.get() == NULL)
		return;

	selectedItems->remove();
}

void
do_save_items()
{
	std::auto_ptr<OPL_TestPoolItemList> selectedItems(
		OPL_TestPoolItemList::getSelected());
	if (selectedItems.get() == NULL)
		return;

	do_save(selectedItems.get(), true, false);
}

void
do_login_details()
{
	OPL_LoginDialog dialog(
		CFSTR("EditLoginDetailsDialog"), CFSTR("Login Details"));

	std::auto_ptr<OPL_TestPoolItemList> selectedItems(
		OPL_TestPoolItemList::getSelected());
	if (selectedItems.get() == NULL)
		return;

	// set initial values (if not multiple selection)
	if (selectedItems->getItemCount() == 1) {
		test_t *test = selectedItems->getItem(0);

		dialog.setEditText(kLoginDSN, OPL_char_to_CFString(
			test->szLoginDSN));
		dialog.setEditText(kLoginUid, OPL_char_to_CFString(
			test->szLoginUID));
		dialog.setEditText(kLoginPwd, OPL_char_to_CFString(
			test->szLoginPWD));
	}

	// run dialog
	if (!dialog.run())
		return;
	
	// fetch dialog data
	char *dsn = OPL_CFString_to_char(dialog.getEditText(kLoginDSN));
	char *uid = OPL_CFString_to_char(dialog.getEditText(kLoginUid));
	char *pwd = OPL_CFString_to_char(dialog.getPasswordEditText(kLoginPwd));
		
	for (UInt32 i = 0; i < selectedItems->getItemCount(); i++) {
		int rc;
		test_t *test = selectedItems->getItem(i);

		strncpy(test->szLoginDSN, dsn, sizeof(test->szLoginDSN) - 1);
		strncpy(test->szLoginUID, uid, sizeof(test->szLoginUID) - 1);
		strncpy(test->szLoginPWD, pwd, sizeof(test->szLoginPWD) - 1); 

		test->tpc.a.uwDrvIdx = -1;
		test->is_dirty = TRUE;
		
		rc = do_login(test);
		if (test->szSQLError[0] || rc != TRUE) {
			pane_log("Connect Error %s: %s\r\n",
				test->szSQLState, test->szSQLError);
			break;
		}
		
		get_dsn_data(test);
		do_logout(test);
	}
	
	free(dsn);
	free(uid);
	free(pwd);
	selectedItems->update();
}

void
do_insert_file()
{
	OPL_TestPool *testPool;
	
	testPool = OPL_TestPool::get();
	if (testPool == NULL)
		return;
		
	do_open(testPool);
}

// Action menu

void
do_create_tables()
{
	std::auto_ptr<OPL_TestPoolItemList> selectedItems(
		OPL_TestPoolItemList::getSelected());
	if (selectedItems.get() == NULL)
		return;

	pane_log("CREATING SCHEMA STARTED\r\n");
	for (UInt32 i = 0; i < selectedItems->getItemCount(); i++) {
		int rc;
		test_t *test = selectedItems->getItem(i);
		
		rc = do_login(test);
		if (!rc || !test->hdbc)
			continue;
			
		switch (test->TestType) {
		case TPC_A:
			fBuildBench(test);
			break;
				
        case TPC_C:
			tpcc_schema_create(NULL, test);
			tpcc_create_db(NULL, test);
			break;
		}

		do_logout(test);
	}
	pane_log("CREATING SCHEMA FINISHED\r\n");
}

void
do_drop_tables()
{
	std::auto_ptr<OPL_TestPoolItemList> selectedItems(
		OPL_TestPoolItemList::getSelected());
	if (selectedItems.get() == NULL)
		return;

	pane_log("CLEANUP STARTED\r\n");
	for (UInt32 i = 0; i < selectedItems->getItemCount(); i++) {
		int rc;
		test_t *test = selectedItems->getItem(i);
		
		rc = do_login(test);
		if (!rc || !test->hdbc)
			continue;
			
		switch (test->TestType) {
		case TPC_A:
			fCleanup(test);
			break;
				
        case TPC_C:
			tpcc_schema_cleanup(NULL, test);
			break;
		}

		do_logout(test);
	}
	pane_log("CLEANUP FINISHED\r\n");
}

#define CMD_RUN_BROWSE	8000
#define CMD_RUN_ALL		8001

static ControlID kRunDuration = { 'RUND', 0 };
static ControlID kRunOutputFile = { 'RUND', 1 };
static ControlID kRunAll = { 'RUND' , 2 };

class OPL_RunDurationDialog: public OPL_Dialog {
public:
	OPL_RunDurationDialog():
		OPL_Dialog(CFSTR("RunDurationDialog")), m_runAll(false)
	{
		// nothing to do
	}

	void init(OPL_TestPoolItemList *selectedItems)
	{
		setEditText(kRunDuration, CFStringCreateCopy(NULL, CFSTR("1")));
		setEditText(kRunOutputFile, CFStringCreateCopy(NULL, CFSTR("results.xml")));

		bool canRunAll = true;
		
		for (UInt32 i = 0; i < selectedItems->getItemCount(); i++) {
			test_t *test = selectedItems->getItem(i);
			
			if (test->TestType != TPC_A)
				canRunAll = false;
		}
		
		if (!canRunAll)
			disableControl(kRunAll);
	}
	
	bool getRunAll()
	{
		return m_runAll;
	}

protected:
	OSStatus handleCommandEvent(UInt32 commandID)
	{
		switch (commandID) {
		case CMD_RUN_BROWSE:
			browseOutputFile();
			return noErr;
			
		case CMD_RUN_ALL:
			m_status = true;
			m_runAll = true;
			endModal();
			return noErr;
	
		default:
			return OPL_Dialog::handleCommandEvent(commandID);
		}
	}

private:
	void browseOutputFile()
	{
		char path[PATH_MAX];
		
		if (!OPL_getSaveFileName(path, sizeof(path),
				CFSTR("Output file"), CFSTR("results.xml")))
			return;
			
		setEditText(kRunOutputFile, OPL_char_to_CFString(path));
	}
	
	bool m_runAll;
};

void
do_run()
{
	std::auto_ptr<OPL_TestPoolItemList> selectedItems(
		OPL_TestPoolItemList::getSelected());
	if (selectedItems.get() == NULL)
		return;

	OPL_RunDurationDialog dialog;

	// init dialog
	dialog.init(selectedItems.get());

	// run dialog
	if (!dialog.run())
		return;

	// run tests
	int nMinutes = dialog.getEditTextIntValue(kRunDuration);
	if (nMinutes < 1)
		return;

	char *filename = OPL_CFString_to_char(dialog.getEditText(kRunOutputFile));
	if (filename[0] == '\0')
		filename = "results.xml";
	else {
		// append .xml
		char *p = strrchr(filename, '/');
		if (p == NULL)
			p = filename;
		p = strchr(p, '.');
		if (p == NULL) {
			size_t len = strlen(filename) + 4 + 1;
			
			p = (char *) malloc(len);
			strlcpy(p, filename, len);
			strlcat(p, ".xml", len);
			free(filename);
			filename = p;
		}
	}
	
	// run selected tests
	OList *item_olist = selectedItems->getAsOList();
	g_cancelled = false;
	do_run_selected(item_olist, selectedItems->getItemCount(),
		filename, nMinutes, dialog.getRunAll());

	// cleanup
	o_list_free(item_olist);
	free(filename);
}

// Results menu

void
do_results_connect()
{
	OPL_LoginDialog dialog(
		CFSTR("EditLoginDetailsDialog"), CFSTR("Results Connect"));
	
	// run dialog
	if (!dialog.run())
		return;
	
	// fetch dialog data
	char *dsn = OPL_CFString_to_char(dialog.getEditText(kLoginDSN));
	char *uid = OPL_CFString_to_char(dialog.getEditText(kLoginUid));
	char *pwd = OPL_CFString_to_char(dialog.getPasswordEditText(kLoginPwd));
	
	results_logout();
	results_login(dsn, uid, pwd);

	free(dsn);
	free(uid);
	free(pwd);
}

void
do_results_disconnect()
{
	results_logout();
}

void
do_results_create_table()
{
	create_results_table();
}

void
do_results_drop_table()
{
	drop_results_table();
}
