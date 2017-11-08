/*
 * Author: doe300
 *
 * See the file "LICENSE" for the full license governing this code.
 */

#ifndef VC4CL_EVENT
#define VC4CL_EVENT

#include <utility>
#include <vector>
#include <memory>
#include <functional>

#include "Object.h"
#include "CommandQueue.h"
#include "extensions.h"


namespace vc4cl
{
	/*
	 * wraps the CL_COMMAND_XXX macros, so they can be used in switch-cases with check for full coverage
	 *
	 * NOTE: only supported cl_command_type constants are listed
	 */
	enum class CommandType
	{
		KERNEL_NDRANGE = CL_COMMAND_NDRANGE_KERNEL,
		KERNEL_TASK = CL_COMMAND_TASK,
		KERNEL_NATIVE = CL_COMMAND_NATIVE_KERNEL,
		BUFFER_READ = CL_COMMAND_READ_BUFFER,
		BUFFER_WRITE = CL_COMMAND_WRITE_BUFFER,
		BUFFER_COPY = CL_COMMAND_COPY_BUFFER,
		IMAGE_READ = CL_COMMAND_READ_IMAGE,
		IMAGE_WRITE = CL_COMMAND_WRITE_IMAGE,
		IMAGE_COPY = CL_COMMAND_COPY_IMAGE,
		IMAGE_COPY_TO_BUFFER = CL_COMMAND_COPY_IMAGE_TO_BUFFER,
		BUFFER_COPY_TO_IMAGE = CL_COMMAND_COPY_BUFFER_TO_IMAGE,
		BUFFER_MAP = CL_COMMAND_MAP_BUFFER,
		IMAGE_MAP = CL_COMMAND_MAP_IMAGE,
		BUFFER_UNMAP = CL_COMMAND_UNMAP_MEM_OBJECT,
		MARKER = CL_COMMAND_MARKER,
		BUFFER_READ_RECT = CL_COMMAND_READ_BUFFER_RECT,
		BUFFER_WRITE_RECT = CL_COMMAND_WRITE_BUFFER_RECT,
		BUFFER_COPY_RECT = CL_COMMAND_COPY_BUFFER_RECT,
		USER_COMMAND = CL_COMMAND_USER,
		BARRIER = CL_COMMAND_BARRIER,
		BUFFER_MIGRATE = CL_COMMAND_MIGRATE_MEM_OBJECTS,
		BUFFER_FILL = CL_COMMAND_FILL_BUFFER,
		IMAGE_FILL = CL_COMMAND_FILL_IMAGE,
		SVM_FREE = CL_COMMAND_SVM_FREE_ARM,
		SVM_MEMCPY = CL_COMMAND_SVM_MEMCPY_ARM,
		SVM_MEMFILL = CL_COMMAND_SVM_MEMFILL_ARM,
		SVM_MAP = CL_COMMAND_SVM_MAP_ARM,
		SVM_UNMAP = CL_COMMAND_SVM_UNMAP_ARM
	};

	enum ProfileIndex
	{
		QUEUE_TIME = 0,
		SUBMIT_TIME = 1,
		START_TIME = 2,
		END_TIME = 3
	};

	struct EventProfile
	{
		cl_ulong queue_time = 0;
		cl_ulong submit_time = 0;
		cl_ulong start_time = 0;
		cl_ulong end_time = 0;
	};

	struct EventAction
	{
	public:
		EventAction();

		virtual ~EventAction();

		CHECK_RETURN virtual cl_int operator()(Event* event) = 0;

	private:
		//prohibit copying or moving, since it might screw up with the manual reference counts
		EventAction(const EventAction&) = delete;
		EventAction(EventAction&&) = delete;

		EventAction& operator=(const EventAction&) = delete;
		EventAction& operator=(EventAction&&) = delete;
	};

	/*
	 * Event source to run a custom function
	 */
	struct CustomAction : public EventAction
	{
		const std::function<cl_int(Event*)> func;

		CustomAction(const std::function<cl_int(Event*)> callback) : func(callback) { }

		cl_int operator()(Event* event) override
		{
			return func(event);
		}
	};

	/*
	 * Event source doing nothing, just returns the given status
	 */
	struct NoAction : public EventAction
	{
		const cl_int status;
		NoAction(cl_int status = CL_SUCCESS) : status(status) { }

		cl_int operator()(Event* event) override
		{
			return status;
		}
	};

	typedef void(CL_CALLBACK* EventCallback)(cl_event event, cl_int event_command_exec_status, void* user_data);

	class Event: public Object<_cl_event, CL_INVALID_EVENT>, public HasContext
	{
	public:
		Event(Context* context, cl_int status, CommandType type);
		~Event();

		CHECK_RETURN cl_int setUserEventStatus(cl_int execution_status);
		CHECK_RETURN cl_int getInfo(cl_event_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret) const;
		CHECK_RETURN cl_int setCallback(cl_int command_exec_callback_type, EventCallback callback, void* user_data);
		CHECK_RETURN cl_int getProfilingInfo(cl_profiling_info param_name, size_t param_value_size, void* param_value, size_t* param_value_size_ret) const;

		CHECK_RETURN cl_int waitFor() const;
		bool isFinished() const;
		cl_int getStatus() const;
		void fireCallbacks();
		void updateStatus(cl_int status, bool fireCallbacks = true);
		CommandQueue* getCommandQueue();
		CHECK_RETURN cl_int prepareToQueue(CommandQueue* queue);
		void setEventWaitList(cl_uint numEvents, const cl_event* events);

		const CommandType type;
		std::unique_ptr<EventAction> action;
	private:
		object_wrapper<CommandQueue> queue;

		cl_int status;
		bool userStatusSet;

		EventProfile profile;
		void setTime(cl_ulong& field);

		std::vector<std::tuple<cl_int, EventCallback, void*>> callbacks;
		std::vector<Event*> waitList;

		friend class CommandQueue;
	};

} /* namespace vc4cl */

#endif /* VC4CL_EVENT */
