#pragma once
namespace xnet
{
	class timer_manager :
		public std::multimap<
		std::chrono::high_resolution_clock::time_point, 
		std::function<void()>
		>
	{
	public:
		typedef iterator timer_id;
		timer_manager()
		{

		}
		int64_t do_timer()
		{
			if (empty())
				return 0;

			auto now = std::chrono::high_resolution_clock::now();
			auto itr = begin();
			while (size() && itr->first <= now)
			{
				itr->second();
				now = std::chrono::high_resolution_clock::now();
				erase(itr);
				if (size())
					itr = begin();
				else
					return 0;
			}
			return std::chrono::duration_cast<
				std::chrono::milliseconds>(itr->first - now).count();
		}
	};
}
