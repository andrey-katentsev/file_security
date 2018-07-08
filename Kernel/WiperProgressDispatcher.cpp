#include "WiperProgressDispatcher.h"

#include "../Common/CommunicatorProgressHandler.h"

namespace KAA
{
	namespace FileSecurity
	{
		WiperProgressDispatcher::WiperProgressDispatcher() :
		communicator_progress(nullptr)
		{}

		WiperProgressDispatcher::WiperProgressDispatcher(std::shared_ptr<CommunicatorProgressHandler> core_progress) :
		communicator_progress(core_progress)
		{}

		std::shared_ptr<CommunicatorProgressHandler> WiperProgressDispatcher::SetProgressHandler(std::shared_ptr<CommunicatorProgressHandler> handler)
		{
			const auto previous = communicator_progress;
			communicator_progress = handler;
			return previous;
		}

		progress_state WiperProgressDispatcher::ichunk_processed(_fsize_t total_processed, _fsize_t total_size)
		{
			if(nullptr != communicator_progress)
				return communicator_progress->PortionProcessed(total_processed, total_size);
			return progress_quiet;
		}
	}
}
