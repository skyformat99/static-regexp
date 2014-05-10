#ifndef __REGEXP__REGEXP2__HPP__
#define __REGEXP__REGEXP2__HPP__

#include <string>
#include <vector>
#include <cstddef>
#include <cstdio>
#include <functional>

//#define DEBUG	
	
#ifdef DEBUG
#define DEBUG_PRINTF(text, ...) { printf("%02zu:" text,string.getPosition(), __VA_ARGS__); }
#define DEBUG_PRINT(text) { printf("%02zu:" text,string.getPosition()); }
#else
#define DEBUG_PRINTF(...)
#define DEBUG_PRINT(text)
#endif

//void printTab(unsigned int deep)
//{
//	while (deep > 0) { printf("  "); deep--; }
//}

namespace SRegExp2 {

	#include "abstraction.hpp"
	
	// available templates for users:
	struct Begin; // ^
	struct End; // $
	
	template <bool positive, wchar_t... codes>  struct CharacterClass; // positive: [abc] or negative: [^abc]
	template <bool positive, wchar_t... rest> struct CharacterRange; // positive [a-b] or negative [^a-b]
	template <wchar_t... codes> struct String;
	using Empty = String<>; // placeholder to options
	template <typename... Parts> struct Sequence; // (abc)
	template <typename... Options> struct Selection; // (a|b|c)
	template <unsigned int min, unsigned int max, typename... Inner> struct Repeat; // a{min;max}
	template <typename... Inner> using Plus = Repeat<1,0,Inner...>; // (abc)+
	template <typename... Inner> using Star = Repeat<0,0,Inner...>; // (abc)*
	template <typename... Inner> using Optional = Selection<Sequence<Inner...>,Empty>; // a?
	template <unsigned int id, typename MemoryType, typename... Inner> struct CatchContent; // catching content of (...)
	template <unsigned int baseid, unsigned int catchid = 0> struct ReCatch; // ([a-z]) \1
	template <typename... Definition> struct RegularExpression; 
	
	// MemoryTypes for CatchContent
	template <size_t size> struct StaticMemory;
	struct DynamicMemory;
	using OneMemory = StaticMemory<1>;
	
	// aliases:
	using Any = CharacterClass<true>; // .
	template <wchar_t... codes> using Set = CharacterClass<true, codes...>;
	template <wchar_t... codes> using NegativeSet = CharacterClass<false, codes...>;
	template <wchar_t... codes> using NegSet = NegativeSet<codes...>;
	template <wchar_t... codes> using Character = CharacterClass<true, codes...>;
	template <wchar_t... codes> using Chr = CharacterClass<true, codes...>;
	template <typename... Parts> using Seq = Sequence<Parts...>;
	template <typename... Options> using Sel = Selection<Options...>;
	template <typename... Inner> using Opt = Optional<Inner...>;
	template <unsigned int id, typename... Inner> using OneCatch = CatchContent<id, OneMemory, Inner...>;
	template <unsigned int id, size_t size, typename... Inner> using StaticCatch = CatchContent<id, StaticMemory<size>, Inner...>;
	template <unsigned int id, typename... Inner> using DynamicCatch = CatchContent<id, DynamicMemory, Inner...>;
	template <wchar_t a, wchar_t b, wchar_t... rest> using CSet = CharacterRange<true, a, b, rest...>;
	template <wchar_t... codes> using Str = String<codes...>;
	// implementation:
	
	struct Catch
	{
		size_t begin;
		size_t len;
		template <typename StringAbstraction, typename Root, typename NearestRight, typename... Right> inline bool match(const StringAbstraction string, size_t & move, unsigned int deep, Root & root, std::reference_wrapper<NearestRight> nright, Right... right)
		{
			// checkpoint => set length
			len = string.getPosition() - begin;
			return nright.get().match(string, move, deep, root, right...);
		}
		template <typename NearestRight, typename... Right> inline void reset(NearestRight &, Right...)
		{
			
		}
	};
	
	struct CatchReturn {
	protected:
		Catch * vdata;
		size_t vsize;
	public:
		size_t size() const {
			return vsize;
		}
		const Catch * data() const {
			return vdata;
		}
		CatchReturn(): vdata{nullptr}, vsize{0} { }
		CatchReturn(Catch * ldata, size_t lsize): vdata{ldata}, vsize{lsize} { }
		Catch * get(const size_t id) {
			if (id < vsize) return &vdata[id];
			else return nullptr;
		}
		Catch * begin() const {
			return vdata;
		}
		Catch * end() const {
			return (vdata + vsize);
		}
	};
	
	template <size_t size> struct StaticMemory
	{
	protected:
		size_t count{0};
		Catch data[size];
	public:
		inline void reset()
		{
			count = 0;
		}
		void set(unsigned int addr, Catch content)
		{
			if (addr < size) data[addr] = content;
		}
		int add(Catch content)
		{
			if (count < size) 
			{
				unsigned int ret = count++;
				data[ret] = content;
				return ret;
			}
			return -1;
		}
		size_t getCount()
		{
			return count;
		}
		CatchReturn getCatches()
		{
			return CatchReturn{data, count};
		}
	};
	
	struct DynamicMemory
	{
	protected:
		std::vector<Catch> data;
	public:
		inline void reset()
		{
			data = {};
		}
		void set(unsigned int addr, Catch content)
		{
			if (addr < getCount()) data[addr] = content;
		}
		int add(Catch content)
		{
			data.push_back(content);
			return data.size()-1;
		}
		size_t getCount()
		{
			return data.size();
		}
		CatchReturn getCatches()
		{
			return CatchReturn{data.data(), getCount()};
		}
	};
	
	struct Closure
	{
		template <typename StringAbstraction, typename Root, typename... Right> inline bool match(const StringAbstraction, size_t &, unsigned int, Root &, Right...)
		{
			return true;
		}
		template <typename NearestRight, typename... Right> inline void reset(std::reference_wrapper<NearestRight>, Right...)
		{
			
		}
		inline void reset()
		{
			
		}
		template <unsigned int id> inline bool get(CatchReturn &) 
		{
			return false;
		}
	};
	
	struct Begin
	{
		template <typename StringAbstraction, typename Root, typename NearestRight, typename... Right> inline bool match(const StringAbstraction string, size_t & move, unsigned int deep, Root & root, std::reference_wrapper<NearestRight> nright, Right... right)
		{
			if (string.isBegin())
			{
				if (nright.get().match(string, move, deep, root, right...))
				{
					return true;
				}
			}
			return false;
		}
		template <typename NearestRight, typename... Right> inline void reset(std::reference_wrapper<NearestRight> nright, Right... right)
		{
			nright.get().reset(right...);
		}
		template <unsigned int id> inline bool get(CatchReturn &) 
		{
			return false;
		}
	};
	
	struct End
	{
		template <typename StringAbstraction, typename Root, typename NearestRight, typename... Right> inline bool match(const StringAbstraction string, size_t & move, unsigned int deep, Root & root, std::reference_wrapper<NearestRight> nright, Right... right)
		{
			if (string.isEnd())
			{
				if (nright.get().match(string, move, deep, root, right...))
				{
					return true;
				}
			}
			return false;
		}
		template <typename NearestRight, typename... Right> inline void reset(std::reference_wrapper<NearestRight> nright, Right... right)
		{
			nright.get().reset(right...);
		}
		template <unsigned int id> inline bool get(CatchReturn &) 
		{
			return false;
		}
	};
	
	template <wchar_t firstCode, wchar_t... codes> struct String<firstCode, codes...>
	{
		template <typename StringAbstraction, typename Root, typename NearestRight, typename... Right> inline bool match(const StringAbstraction string, size_t & move, unsigned int deep, Root & root, std::reference_wrapper<NearestRight> nright, Right... right)
		{
			size_t pos{0};
			if (checkString(string, pos, deep))
			{
				DEBUG_PRINTF("checkString match (pos = %zu)\n",pos);
				size_t pos2{0};
				if (nright.get().match(string.add(pos), pos2, deep, root, right...))
				{
					move = pos+pos2;
					return true;
				}
				DEBUG_PRINT("nright not match.\n");
			}
			else
			{
				DEBUG_PRINTF("String: Character '%c' is different to requested character (first: '%c') .get(). NOT match\n", *string.str, firstCode);
			}
			return false;
		}
		template <typename StringAbstraction> static inline bool checkString(const StringAbstraction string, size_t & move, unsigned int deep)
		{
			size_t pos{0};
			if (string.equal(firstCode))
			{
				DEBUG_PRINTF("Character '%c' compared with '%c' .get(). match\n", *string.str, firstCode);
				if (String<codes...>::checkString(string.add(1), pos, deep))
				{
					move = pos+1;
					return true;
				}
			}
			else
			{
				DEBUG_PRINTF("Character '%c' compared with '%c' .get(). NOT match\n", *string.str, firstCode);
			}
			return false;
		}
		template <typename NearestRight, typename... Right> inline void reset(std::reference_wrapper<NearestRight> nright, Right... right)
		{
			nright.get().reset(right...);
		}
		template <unsigned int id> inline bool get(CatchReturn &) 
		{
			return false;
		}
	};
	
	template <> struct String<>
	{
		template <typename StringAbstraction, typename Root, typename NearestRight, typename... Right> inline bool match(const StringAbstraction string, size_t & move, unsigned int deep, Root & root, std::reference_wrapper<NearestRight> nright, Right... right)
		{
			return nright.get().match(string, move, deep, root, right...);
		}
		template <typename StringAbstraction> static inline bool checkString(const StringAbstraction, size_t &, unsigned int)
		{
			return true;
		}
		template <typename NearestRight, typename... Right> inline void reset(std::reference_wrapper<NearestRight> nright, Right... right)
		{
			nright.get().reset(right...);
		}
		template <unsigned int id> inline bool get(CatchReturn &) 
		{
			return false;
		}
	};
	
	template <bool positive, wchar_t a, wchar_t b, wchar_t... rest> struct CharacterRange<positive, a, b, rest...>
	{
		static const constexpr bool isEmpty{false};
		template <typename StringAbstraction, typename Root, typename NearestRight, typename... Right> inline bool match(const StringAbstraction string, size_t & move, unsigned int deep, Root & root, std::reference_wrapper<NearestRight> nright, Right... right)
		{
			if ((positive && checkChar(string, deep)) || (!positive && !checkChar(string, deep)))
			{
				size_t pos{0};
				if (nright.get().match(string.add(1), pos, deep, root, right...))
				{
					move = pos+1;
					return true;
				}
				else
				{
					reset(nright, right...);
				}
			}
			else
			{
				DEBUG_PRINTF("Character '%c' is different to requested character ('%c' - '%c') .get(). NOT match\n", *string.str, a, b);
			}
			return false;
		}
		template <typename StringAbstraction> static inline bool checkChar(const StringAbstraction string, unsigned int deep)
		{
			if (string.charIsBetween(a,b)) 
			{
				DEBUG_PRINTF("Character compared with '%c'-'%c' .get(). match\n", a,b);
				return true;
			}
			else if (!CharacterRange<positive, rest...>::isEmpty && CharacterRange<positive, rest...>::checkChar(string, deep)) return true;
			else return false;
		}
		template <typename NearestRight, typename... Right> inline void reset(std::reference_wrapper<NearestRight> nright, Right... right)
		{
			nright.get().reset(right...);
		}
		template <unsigned int id> inline bool get(CatchReturn &) 
		{
			return false;
		}
	};
	
	template <bool positive> struct CharacterRange<positive>
	{
		static const constexpr bool isEmpty{true};
		template <typename StringAbstraction, typename Root, typename NearestRight, typename... Right> inline bool match(const StringAbstraction string, size_t & move, unsigned int deep, Root & root, std::reference_wrapper<NearestRight> nright, Right... right)
		{
			if ((positive && checkChar(string, deep)) || (!positive && !checkChar(string, deep)))
			{
				size_t pos{0};
				if (nright.get().match(string.add(1), pos, deep, root, right...))
				{
					move = pos+1;
					return true;
				}
				else
				{
					reset(nright, right...);
				}
			}
			else
			{
				DEBUG_PRINTF("Character '%c' is different to requested character .get(). NOT match\n", *string.str);
			}
			return false;
		}
		template <typename StringAbstraction> static inline bool checkChar(const StringAbstraction string, unsigned int)
		{
			if (string.exists()) 
			{
				DEBUG_PRINT("Character compared with ANY .get(). match\n");
				return true;
			}
			return false;
		}
		template <typename NearestRight, typename... Right> inline void reset(std::reference_wrapper<NearestRight> nright, Right... right)
		{
			nright.get().reset(right...);
		}
		template <unsigned int id> inline bool get(CatchReturn &) 
		{
			return false;
		}
	};	
	
	template <bool positive, wchar_t firstCode, wchar_t... code> struct CharacterClass<positive, firstCode, code...>
	{
		static const constexpr bool isEmpty{false};
		template <typename StringAbstraction, typename Root, typename NearestRight, typename... Right> inline bool match(const StringAbstraction string, size_t & move, unsigned int deep, Root & root, std::reference_wrapper<NearestRight> nright, Right... right)
		{
			if ((positive && checkChar(string, deep)) || (!positive && !checkChar(string, deep)))
			{
				size_t pos{0};
				if (nright.get().match(string.add(1), pos, deep, root, right...))
				{
					move = pos+1;
					return true;
				}
				else
				{
					reset(nright, right...);
				}
			}
			else
			{
				DEBUG_PRINTF("Character '%c' is different to requested character (first: '%c') .get(). NOT match\n", *string.str, firstCode);
			}
			return false;
		}
		template <typename StringAbstraction> static inline bool checkChar(const StringAbstraction string, unsigned int deep)
		{
			if (string.equal(firstCode)) 
			{
				DEBUG_PRINTF("Character compared with '%c' .get(). match\n", firstCode);
				return true;
			}
			else if (!CharacterClass<positive, code...>::isEmpty && CharacterClass<positive, code...>::checkChar(string, deep)) return true;
			else return false;
		}
		template <typename NearestRight, typename... Right> inline void reset(std::reference_wrapper<NearestRight> nright, Right... right)
		{
			nright.get().reset(right...);
		}
		template <unsigned int id> inline bool get(CatchReturn &) 
		{
			return false;
		}
	};
	
	template <bool positive> struct CharacterClass<positive>
	{
		static const constexpr bool isEmpty{true};
		template <typename StringAbstraction, typename Root, typename NearestRight, typename... Right> inline bool match(const StringAbstraction string, size_t & move, unsigned int deep, Root & root, std::reference_wrapper<NearestRight> nright, Right... right)
		{
			if ((positive && checkChar(string, deep)) || (!positive && !checkChar(string, deep)))
			{
				size_t pos{0};
				if (nright.get().match(string.add(1), pos, deep, root, right...))
				{
					move = pos+1;
					return true;
				}
				else
				{
					reset(nright, right...);
				}
			}
			else
			{
				DEBUG_PRINTF("Character '%c' is different to requested character .get(). NOT match\n", *string.str);
			}
			return false;
		}
		template <typename StringAbstraction> static inline bool checkChar(const StringAbstraction string, unsigned int)
		{
			if (string.exists()) 
			{
				DEBUG_PRINT("Character compared with ANY .get(). match\n");
				return true;
			}
			return false;
		}
		template <typename NearestRight, typename... Right> inline void reset(std::reference_wrapper<NearestRight> nright, Right... right)
		{
			nright.get().reset(right...);
		}
		template <unsigned int id> inline bool get(CatchReturn &) 
		{
			return false;
		}
	};
	
	template <unsigned int id, typename MemoryType, typename Inner, typename... Rest> struct CatchContent<id, MemoryType, Inner, Rest...>
	{
		CatchContent<id, Seq<Inner,Rest...>> innerContent;
		template <typename StringAbstraction, typename Root, typename NearestRight, typename... Right> inline bool match(const StringAbstraction string, size_t & move, unsigned int deep, Root & root, std::reference_wrapper<NearestRight> nright, Right... right)
		{
			return innerContent.match(string, move, deep, root, nright, right...);
		}
		template <typename NearestRight, typename... Right> inline void reset(std::reference_wrapper<NearestRight> nright, Right... right)
		{
			innerContent.reset(nright, right...);
		}
		template <unsigned int subid> inline bool get(CatchReturn & catches) 
		{
			return innerContent.template get<subid>(catches);
		}
	};
	
	template <unsigned int id, typename MemoryType, typename Inner> struct CatchContent<id, MemoryType, Inner>
	{
		struct Mark
		{
			size_t begin;
			size_t len;
			int addr{-1};
			MemoryType & memory;
			template <typename StringAbstraction, typename Root, typename NearestRight, typename... Right> inline bool match(const StringAbstraction string, size_t & move, unsigned int deep, Root & root, std::reference_wrapper<NearestRight> nright, Right... right)
			{
				// checkpoint => set length
				len = string.getPosition() - begin;
				if (addr >= 0)
				{
					memory.set(addr,{begin,len});
				}
				else
				{
					addr = memory.add({begin,len});
				}
				return nright.get().match(string, move, deep, root, right...);
				
			}
			template <typename NearestRight, typename... Right> inline void reset(NearestRight &, Right...)
			{
			
			}
			Mark(size_t lbegin, MemoryType & lmemory): begin{lbegin}, len{0}, memory{lmemory}
			{
				
			}
		};
		
		Inner inner;
		MemoryType memory;
		template <typename StringAbstraction, typename Root, typename NearestRight, typename... Right> inline bool match(const StringAbstraction string, size_t & move, unsigned int deep, Root & root, std::reference_wrapper<NearestRight> nright, Right... right)
		{
			Mark mark{string.getPosition(), memory};
			bool ret{inner.match(string, move, deep, root, std::ref(mark), nright, right...)};
			if (!ret)
			{
				memory.reset();
				reset(nright, right...);
			}
			return ret;
		}
		template <typename NearestRight, typename... Right> inline void reset(std::reference_wrapper<NearestRight> nright, Right... right)
		{
			inner.reset(nright, right...);
		}
		template <unsigned int subid> inline bool get(CatchReturn & catches) 
		{
			if (subid == id) 
			{
				//printf("here! size = %zu\n",memory.getCount());
				catches = memory.getCatches();
				return true;
			}
			else return inner.template get<id>(catches);
		}
	};
	
	template <unsigned int baseid, unsigned int catchid> struct ReCatch
	{
		template <typename StringAbstraction, typename Root, typename NearestRight, typename... Right> inline bool match(const StringAbstraction string, size_t & move, unsigned int deep, Root & root, std::reference_wrapper<NearestRight> nright, Right... right)
		{
			CatchReturn ret;
			if (root.template get<baseid>(ret)) {
				//printf("catch found (size = %zu)\n",ret.size());
				const Catch * ctch{ret.get(catchid)};
				if (ctch) {
					//printf("subcatch found\n");
					for (size_t l{0}; l != ctch->len; ++l) {
						if (!string.equalToOriginal(ctch->begin+l,l)) return false;
					}
					size_t tmp{0};
					if (nright.get().match(string.add(ctch->len), tmp, deep, root, right...))
					{
						move += ctch->len + tmp;
						return true;
					}
					else
					{
						reset(nright, right...);
					}
				}
				else
				{
					//printf("subcatch NOT found %u\n",catchid);
				}
			}
			return false;
		}
		template <typename NearestRight, typename... Right> inline void reset(std::reference_wrapper<NearestRight> nright, Right... right)
		{
			nright.get().reset(right...);
		}
		template <unsigned int subid> inline bool get(CatchReturn &) 
		{
			return false;
		}
	};
	
	template <typename FirstOption, typename... Options> struct Selection<FirstOption, Options...>
	{
		FirstOption first;
		Selection<Options...> restopt;
		template <typename StringAbstraction, typename Root, typename NearestRight, typename... Right> inline bool match(const StringAbstraction string, size_t & move, unsigned int deep, Root & root, std::reference_wrapper<NearestRight> nright, Right... right)
		{
			if (first.match(string, move, deep+1, root, nright, right...))
			{
				return true;
			}
			else
			{
				first.reset(nright, right...);
				return restopt.match(string, move, deep+1, root, nright, right...);
			}
		}
		template <typename NearestRight, typename... Right> inline void reset(std::reference_wrapper<NearestRight> nright, Right... right)
		{
			first.reset(nright, right...);
			restopt.reset(nright, right...);
		}
		template <unsigned int id> inline bool get(CatchReturn & catches) 
		{
			return first.template get<id>(catches) || restopt.template get<id>(catches);
		}
	};
	
	template <> struct Selection<>
	{
		template <typename StringAbstraction, typename Root, typename NearestRight, typename... Right> inline bool match(const StringAbstraction, size_t &, unsigned int, Root &, std::reference_wrapper<NearestRight>, Right...)
		{
			return false;
		}
		template <typename NearestRight, typename... Right> inline void reset(NearestRight &, Right...)
		{
			
		}
		template <unsigned int id> inline bool get(CatchReturn &) 
		{
			return false;
		}
	};
	
	template <typename First, typename... Rest> struct Sequence<First, Rest...>
	{
		First first;
		Sequence<Rest...> restseq;
		template <typename StringAbstraction, typename Root, typename NearestRight, typename... Right> inline bool match(const StringAbstraction string, size_t & move, unsigned int deep, Root & root, std::reference_wrapper<NearestRight> nright, Right... right)
		{
			if (first.match(string, move, deep, root, std::ref(restseq), nright, right...))
			{
				return true;
			}
			first.reset(nright, right...);
			return false;
		}
		template <typename NearestRight, typename... Right> inline void reset(std::reference_wrapper<NearestRight> nright, Right... right)
		{
			first.reset(std::ref(restseq), nright, right...);
		}
		template <unsigned int id> inline bool get(CatchReturn & catches) 
		{
			return first.template get<id>(catches) || restseq.template get<id>(catches);
		}
	};

	template <typename First> struct Sequence<First>
	{
		First first;
		template <typename StringAbstraction, typename Root, typename NearestRight, typename... Right> inline bool match(const StringAbstraction string, size_t & move, unsigned int deep, Root & root, std::reference_wrapper<NearestRight> nright, Right... right)
		{
			if (first.match(string, move, deep, root, nright, right...))
			{
				return true;
			}
			first.reset(nright, right...);
			return false;
		}
		template <typename NearestRight, typename... Right> inline void reset(std::reference_wrapper<NearestRight> nright, Right... right)
		{
			first.reset(nright, right...);
		}
		template <unsigned int id> inline bool get(CatchReturn & catches) 
		{
			return first.template get<id>(catches);
		}
	};
	
	template <unsigned int min, unsigned int max, typename Inner, typename... Rest> struct Repeat<min, max, Inner, Rest...>
	{
		Repeat<min, max, Seq<Inner,Rest...>> innerRepeat;
		template <typename StringAbstraction, typename Root, typename NearestRight, typename... Right> inline bool match(const StringAbstraction string, size_t & move, unsigned int deep, Root & root, std::reference_wrapper<NearestRight> nright, Right... right)
		{
			return innerRepeat.match(string, move, deep, root, nright, right...);
		}
		template <typename NearestRight, typename... Right> inline void reset(std::reference_wrapper<NearestRight> nright, Right... right)
		{
			innerRepeat.reset(nright, right...);
		}
		template <unsigned int id> inline bool get(CatchReturn & catches) 
		{
			return innerRepeat.template get<id>(catches);
		}
	};
	
	template <unsigned int min, unsigned int max, typename Inner> struct Repeat<min, max, Inner>
	{
		Inner inner;
		template <typename StringAbstraction, typename Root, typename NearestRight, typename... Right> inline bool match(const StringAbstraction string, size_t & move, unsigned int deep, Root & root, std::reference_wrapper<NearestRight> nright, Right... right)
		{
			size_t pos{0};
			ssize_t lastFound{-1};
			Closure closure;
			// TODO check if this is necessary
			//Inner copyOfInner(inner);
			//NearestRight copyOfNearestRight(nright.get());
			for (size_t cycle{0}; (!max) || (cycle < max); ++cycle)
			{
				size_t tmp{0};
				if (nright.get().match(string.add(pos), tmp, deep+1, root, right...) && (cycle >= min))
				{
					//copyOfNearestRight = nright.get();
					lastFound = pos + tmp;
					DEBUG_PRINTF(">> found at %zu\n",lastFound);
					pos += tmp;
				}
				// in next expression "empty" is needed
				if (inner.match(string.add(pos), tmp, deep+1, root, std::ref(closure)))
				{
					//copyOfInner = inner;
					pos += tmp;
				}
				else 
				{
					if (lastFound >= 0)
					{
						//inner = copyOfInner;
						//nright = copyOfNearestRight;
						DEBUG_PRINTF("cycle done (cycle = %zu)\n",cycle);
						move += static_cast<size_t>(lastFound);
						return true;
					}
					else break;
				}
				
			}
			return false;
		}
		template <typename NearestRight, typename... Right> inline void reset(std::reference_wrapper<NearestRight> nright, Right... right)
		{
			inner.reset(nright, right...);
		}
		template <unsigned int id> inline bool get(CatchReturn & catches) 
		{
			return inner.template get<id>(catches);
		}
	};
	
	template <typename... Inner> struct Eat
	{
		Sequence<Inner...> inner;
		template <typename StringAbstraction, typename Root, typename NearestRight, typename... Right> inline bool match(const StringAbstraction string, size_t & move, unsigned int deep, Root & root, std::reference_wrapper<NearestRight> nright, Right... right)
		{
			size_t pos{0};
			
			if (!string.exists() && inner.match(string, move, deep+1, root, nright, right...))
			{
				// branch just for empty strings
				return true;
			}
			else while (string.exists(pos)) {
				size_t imove{0};
				//DEBUG_PRINTF("eating... (pos = %zu)\n",pos);
				if (inner.match(string.add(pos), imove, deep+1, root, nright, right...))
				{
					move += pos + imove;
					return true;
				}
				else pos++;
			}
			return false;
		}
		template <typename NearestRight, typename... Right> inline void reset(std::reference_wrapper<NearestRight> nright, Right... right)
		{
			inner.reset(nright, right...);
		}
		template <unsigned int id> inline bool get(CatchReturn & catches) 
		{
			return inner.template get<id>(catches);
		}
	};
	
	template <typename... Definition> struct RegularExpression
	{
		Eat<Definition...> eat;
		bool operator()(std::string string)
		{
			size_t pos{0};
			Closure closure;
			return eat.match(StringAbstraction<const char *>(string.c_str()), pos, 0, eat, std::ref(closure));
		}
		bool operator()(const char * string)
		{
			size_t pos{0};
			Closure closure;
			return eat.match(StringAbstraction<const char *>(string), pos, 0, eat, std::ref(closure));
		}
		template <unsigned int id> CatchReturn get()
		{
			CatchReturn catches;
			eat.template get<id>(catches);
			return catches;
		}
		template <typename StringType> static constexpr bool smatch(StringType string)
		{
			RegularExpression<Definition...> regexp{};
			return regexp(string);
		}
	};
}

#endif