/**
 *
 */

#ifndef auto_ptr_fix_h_
#define auto_ptr_fix_h_

#include <memory>

namespace rr {
	template<typename T>
	struct auto_ptr {
		T* val{nullptr};
		T* release() {
			T* v = val;
			val = 0;
			return v;
		}

		auto_ptr<T>& operator=(auto_ptr<T>& other)
		{
			reset(other.get());
			return *this;
		}

		void reset(T* ptr)
		{
			//if (val != nullptr) delete val;

			val = ptr;
		}

		~auto_ptr() {
			//if (val)
			//{
			//	delete val;
			//}
			
		}

		T* operator->() const {
			return val;
		}
		T* get() {
			return val;
		}
	};
}


template <class T_>
inline void auto_ptr_copy (rr::auto_ptr<T_>& lhs, rr::auto_ptr<T_> rhs) {
	lhs.val = rhs.val;
}
template <class T_>
inline void auto_ptr_copy (rr::auto_ptr<T_>& lhs, T_* rhs) {
	//rr::auto_ptr<T_> p(rhs);
	lhs.reset(rhs);// = p;
}
template <class T_>
inline T_* auto_ptr_release(rr::auto_ptr<T_>& p) {
	//T_* ret = p.release();
	//auto_ptr_copy(p, rr::auto_ptr<T_>());
	return p.release();
}

#endif // ! auto_ptr_fix_h_

