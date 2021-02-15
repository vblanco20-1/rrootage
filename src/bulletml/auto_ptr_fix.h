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
	lhs = rhs;
}
template <class T_>
inline void auto_ptr_copy (rr::auto_ptr<T_>& lhs, T_* rhs) {
	//rr::auto_ptr<T_> p(rhs);
	lhs.val = rhs;// = p;
}
template <class T_>
inline T_* auto_ptr_release(rr::auto_ptr<T_>& p) {
	T_* ret = p.release();
	auto_ptr_copy(p, rr::auto_ptr<T_>());
	return ret;
}

#endif // ! auto_ptr_fix_h_

