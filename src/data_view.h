#pragma once

namespace stdex {
	template <typename T>
	class data_view
	{
	public:
		using value_type = T;
		using const_pointer = const T*;
		using const_iterator = const_pointer;

		data_view()
			:ptr_(nullptr), size_(0)
		{

		}

		data_view(const T* ptr, size_t size)
			:ptr_(ptr), size_(size)
		{

		}

		~data_view() = default;

		bool empty() const
		{
			return size_ == 0;
		}

		size_t size() const
		{
			return size_;
		}

		const_pointer data()const
		{
			return ptr_;
		}
		const_iterator begin()const
		{
			return ptr_;
		}
		const_iterator end()const
		{
			return ptr_ + size_;
		}


	private:
		const T* ptr_;
		size_t size_;
	};


	template <typename T>
	data_view<T> make_data_view(const T* ptr, size_t size)
	{
		return data_view<T>(ptr, size);
	}

}
