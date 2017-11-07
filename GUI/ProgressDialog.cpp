#include "KAA/include/keywords.h"
#include "KAA/include/load_string.h"

#include <process.h>

#include "ClientCommunicator.h"
#include "OperationContext.h"

#include "../Common/CommunicatorProgressHandler.h"
#include "../Common/UserReport.h"

#include <windows.h>
#include <commctrl.h>

#include "resource.h"

namespace KAA
{
	namespace user_interface
	{
		enum { marquee_on = TRUE, marquee_off = FALSE };
	}
}

namespace
{
	unsigned short PercentComplete(const uint64_t total_processed, const uint64_t total_size)
	{
		const double complete = static_cast<double>(total_processed) / total_size;
		return static_cast<unsigned short>(complete * 100);
	}

	class ProgressDialogHandler : public KAA::FileSecurity::CommunicatorProgressHandler
	{
	public:
		ProgressDialogHandler(const HWND dialog) :
		dialog(dialog)
		{}

	private:
		HWND dialog;

		KAA::progress_state IOperationStarted(const std::wstring& name) override
		{
			::SetDlgItemTextW(dialog, IDC_PROGRESS_CURRENT_OPERATION_STATIC, name.c_str());
			{
				const HWND progress = ::GetDlgItem(dialog, IDC_PROGRESS_TOTAL_PROGRESS);
				//::SendMessageW(progress, PBM_SETPOS, 0, 0);
			}
			return KAA::progress_continue;
		}

		KAA::progress_state IPortionProcessed(uint64_t total_processed, uint64_t total_size) override
		{ // DEFECT: KAA: extra call.
			{
				const HWND progress = ::GetDlgItem(dialog, IDC_PROGRESS_TOTAL_PROGRESS);
				::SendMessageW(progress, PBM_SETPOS, PercentComplete(total_processed, total_size), 0);
			}
			return KAA::progress_continue;
		}

		KAA::progress_state IOverallProgress(uint64_t total_processed, uint64_t total_size) override
		{
			return KAA::progress_continue;
		}
	};

	// DEFECT: KAA: global data.
	INT_PTR g_result = IDOK;

	void ExecuteTask(void (*task)(void*), void* context)
	{
		(*task)(context);
	}

	void ExecuteTask(const HWND dialog, KAA::FileSecurity::OperationContext* task)
	{
		//INT_PTR result = IDOK;
		g_result = IDOK;

		ProgressDialogHandler progress_handler(dialog);
		KAA::FileSecurity::CommunicatorProgressHandler* previous = KAA::FileSecurity::GetCommunicator().SetProgressHandler(&progress_handler); // FIX: KAA: provide RAII.

		try
		{
			ExecuteTask(task->m_requested_operation, task->m_context);

			{
				const HWND progress = ::GetDlgItem(dialog, IDC_PROGRESS_TOTAL_PROGRESS);
				//::SendMessageW(progress, PBM_SETPOS, 100, 0); // DEFECT: KAA: no callback for simple remove operation.
			}
			// DEFECT: KAA: DRY violation.
			{
				const std::wstring message(KAA::resources::load_string(task->success_message_id));
				::SetDlgItemTextW(dialog, IDC_PROGRESS_CURRENT_OPERATION_STATIC, message.c_str());
			}
			{
				const std::wstring text(KAA::resources::load_string(IDS_FINISH_BUTTON_TITLE));
				::SetDlgItemTextW(dialog, IDC_PROGRESS_STOP_BUTTON, text.c_str());
			}
		}
		catch(const KAA::FileSecurity::UserReport& report)
		{
			//result = IDABORT;
			g_result = IDCANCEL;

			const std::wstring title(KAA::resources::load_string(task->operation_name_id));
			::MessageBoxW(dialog, report.format_message().c_str(), title.c_str(), MB_OK | ToIconID(report.severity()));

			{
				const std::wstring message(KAA::resources::load_string(task->failure_message_id));
				::SetDlgItemTextW(dialog, IDC_PROGRESS_CURRENT_OPERATION_STATIC, message.c_str());
			}
			{
				const std::wstring text(KAA::resources::load_string(IDS_CLOSE_BUTTON_TITLE));
				::SetDlgItemTextW(dialog, IDC_PROGRESS_STOP_BUTTON, text.c_str());
			}
		}

		KAA::FileSecurity::GetCommunicator().SetProgressHandler(previous);

		{
			//const HWND progress = ::GetDlgItem(dialog, IDC_PROGRESS_TOTAL_PROGRESS);
			//::SendMessageW(progress, PBM_SETMARQUEE, KAA::user_interface::marquee_off, 0);
		}

		::EnableWindow(::GetDlgItem(dialog, IDC_PROGRESS_STOP_BUTTON), TRUE);
		//::EndDialog(dialog, result);
	}

	struct ExecuteTaskContext
	{
		HWND dialog;
		KAA::FileSecurity::OperationContext* context;
	};

	void ExecuteTask(void* context)
	{
		const std::auto_ptr<ExecuteTaskContext> task_context(reinterpret_cast<ExecuteTaskContext*>(context));
		ExecuteTask(task_context->dialog, task_context->context);
	}

	BOOL ProcessControlMessage(const HWND dialog, const HWND control, const int control_identifier, const int notification_code)
	{
		switch(control_identifier)
		{
		case IDC_PROGRESS_STOP_BUTTON:
			{
				switch(notification_code)
				{
				case BN_CLICKED:
					{
						//::EndDialog(dialog, IDCANCEL);
						::EndDialog(dialog, g_result);
					} break;
				default:
					return FALSE;
				}
			} break;
		default:
			return FALSE;
		}
		return TRUE;
	}

	BOOL ProcessMenuMessage(const HWND, const int)
	{ return FALSE; }

	BOOL ProcessAcceleratorMessage(const HWND, const int)
	{ return FALSE; }

	// If an application processes this message, it should return zero.
	INT_PTR OnClose(const HWND dialog)
	{
		::SetWindowLongPtrW(dialog, DWL_MSGRESULT, 0L);
		return TRUE;
	}

	// If an application processes this message, it should return zero.
	INT_PTR OnCommand(const HWND dialog, const WPARAM message_context, const LPARAM extended_message_context)
	{
		const HWND control = reinterpret_cast<HWND>(extended_message_context);
		const int notification_code = HIWORD(message_context);
		const int id = LOWORD(message_context);

		BOOL message_processed = FALSE;

		if(nullptr != control)
		{
			message_processed = ProcessControlMessage(dialog, control, id, notification_code);
		}
		else
		{
			if(0 == notification_code)
				message_processed = ProcessMenuMessage(dialog, id);
			else
				message_processed = ProcessAcceleratorMessage(dialog, id);
		}

		if(TRUE == message_processed)
			::SetWindowLongPtrW(dialog, DWL_MSGRESULT, 0L);

		return message_processed;
	}

	// No return value.
	INT_PTR OnHotKey(const HWND dialog, const WPARAM message_context, const LPARAM extended_message_context)
	{
		const int hot_key_identifier = message_context;
		const WORD virtual_key_code = HIWORD(extended_message_context);
		const WORD modifiers = LOWORD(extended_message_context); // any combination of MOD_ALT, MOD_CONTROL, MOD_SHIFT, MOD_WIN

		switch(hot_key_identifier)
		{
		case IDHOT_SNAPDESKTOP: // system-defined hot key
		case IDHOT_SNAPWINDOW: // system-defined hot key
		default: break;
		}

		return FALSE;
	}

	// The dialog box procedure should return TRUE to direct the system to set the keyboard focus to the control specified by wParam.
	// Otherwise, it should return FALSE to prevent the system from setting the default keyboard focus.
	// The dialog box procedure should return the value directly. The DWL_MSGRESULT value set by the SetWindowLong function is ignored.
	INT_PTR OnInitDialog(const HWND dialog, const WPARAM focus_default, const LPARAM initialization_data)
	{
		//enum { update_time_in_milliseconds = 50 };

		//const HWND progress = ::GetDlgItem(dialog, IDC_PROGRESS_TOTAL_PROGRESS);
		{
			//const LONG_PTR default_style = ::GetWindowLongPtrW(progress, GWL_STYLE);
			//const LONG_PTR marquee = default_style | PBS_MARQUEE;
			//::SetWindowLongPtrW(progress, GWL_STYLE, marquee);
		}
		//::SendMessageW(progress, PBM_SETMARQUEE, KAA::user_interface::marquee_on, update_time_in_milliseconds);

		{
			KAA::FileSecurity::OperationContext* context = reinterpret_cast<KAA::FileSecurity::OperationContext*>(initialization_data);

			std::auto_ptr<ExecuteTaskContext> task_context(new ExecuteTaskContext);
			task_context->dialog = dialog;
			task_context->context =  context;
			_beginthread(ExecuteTask, 0, task_context.get()); // FUTURE: KAA: introduce SDK beginthread (throw on failure).
			task_context.release(); // DEFECT: KAA: memory-leak possible.
		}

		return TRUE;
	}
}

// Typically, the dialog box procedure should return TRUE if it processed the message, and FALSE if it did not. If the dialog box procedure returns FALSE, the dialog manager performs the default dialog operation in response to the message.
// If the dialog box procedure processes a message that requires a specific return value, the dialog box procedure should set the icon_size return value by calling SetWindowLong(hwndDlg, DWL_MSGRESULT, lResult) immediately before returning TRUE. Note that you must call SetWindowLong immediately before returning TRUE; doing so earlier may result in the DWL_MSGRESULT value being overwritten by a nested dialog box message.
INT_PTR CALLBACK ProgressDialog(HWND dialog, UINT message, WPARAM message_context, LPARAM extended_message_context)
{
	switch(message)
	{
	case WM_CLOSE:
		return OnClose(dialog);
	case WM_COMMAND:
		return OnCommand(dialog, message_context, extended_message_context);
	case WM_HOTKEY:
		return OnHotKey(dialog, message_context, extended_message_context);
	case WM_INITDIALOG:
		return OnInitDialog(dialog, message_context, extended_message_context);
	default:
		return FALSE;
	}
}
