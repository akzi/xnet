#pragma once
namespace xnet
{
	class guard
	{
	public:
		guard(std::function<void()> &&func)
			:func_(std::move(func))
		{

		}
		~guard()
		{
			if (func_)
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