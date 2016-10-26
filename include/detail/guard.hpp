#pragma once
namespace xnet
{
namespace
{
	template<typename FUNC>
	class guard
	{
	public:
		guard(FUNC &&func)
			:func_(std::move(func))
		{

		}
		~guard()
		{
			if(func_)
				func_();	
		}
		void reset()
		{
			func_ = nullptr;
		}
	private:
		std::function<void()> func_;
	};
}
}