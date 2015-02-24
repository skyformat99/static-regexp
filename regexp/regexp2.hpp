// The MIT License (MIT)
// 
// Copyright (c) 2014-2015 Hana Dusíková (hanicka@hanicka.net)
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
// the Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#ifndef __REGEXP__REGEXP__HPP__
#define __REGEXP__REGEXP__HPP__

#include <string>
#include <vector>
#include <cstddef>
#include <cstdio>
#include <functional>
#include <iostream>
#include <sstream>

//#define DEBUG	
	
#ifdef DEBUG
#define DEBUG_PRINTF(text, ...) { printf("%02zu:" text,string.getPosition(), __VA_ARGS__); }
#define DEBUG_PRINT(text) { printf("%02zu:" text,string.getPosition()); }
#else
#define DEBUG_PRINTF(...)
#define DEBUG_PRINT(text)
#endif

#pragma GCC diagnostic ignored "-Wtype-limits"

//void printTab(unsigned int deep)
//{
//	while (deep > 0) { printf("  "); deep--; }
//}

#include "abstraction.hpp"
#include "support.hpp"

namespace SRX {
	
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
	template <unsigned int min, unsigned int max, typename... Inner> struct XRepeat; // a{min;max}
	template <typename... Inner> using Plus = Repeat<1,0,Inner...>; // (abc)+
	template <typename... Inner> using Star = Repeat<0,0,Inner...>; // (abc)*
	template <typename... Inner> using Optional = Selection<Sequence<Inner...>,Empty>; // a?
	template <unsigned int id, typename MemoryType, typename... Inner> struct CatchContent; // catching content of (...)
	template <unsigned int baseid, unsigned int catchid = 0> struct ReCatch; // ([a-z]) \1
	template <unsigned int baseid, unsigned int catchid = 0> struct ReCatchReverse; // ([a-z]) \1
	template <typename... Definition> struct RegularExpression; 
	
	// MemoryTypes for CatchContent
	template <size_t size> struct StaticMemory;
	struct DynamicMemory;
	using OneMemory = StaticMemory<1>;
	
	// identifier
	template <unsigned int key, unsigned int value> struct Identifier;
	
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
	template <wchar_t a, wchar_t b, wchar_t... rest> using CRange = CharacterRange<true, a, b, rest...>;
	template <wchar_t... codes> using Str = String<codes...>;
	template <unsigned int key, unsigned int value> using Id = Identifier<key,value>;
	using Space = Chr<' '>;
	using WhiteSpace = Set<' ','\t','\r','\n'>;
	using Number = CRange<'0','9'>;
	
	
	
	template <typename CharType> using CompareFnc = bool (*)(const CharType, const CharType, const CharType);
	
	// Checking if subregexp is sequence
	
	template <typename T> struct IsSequence { static constexpr bool value = false; };
	
	// implementation:
	template <typename T> struct Reference
	{
		T & target;
		inline Reference(T & ltarget): target(ltarget) { }
		inline T & getRef() { return target; }
	};
	
	struct CatchReturn;
	
	// last item in recursively called regexp which always return true for match call
	// it must be always used as last item of call-chain
	template <bool artifical = false> struct Closure
	{
		template <typename StringAbstraction, typename Root, typename... Right> inline bool match(const StringAbstraction, size_t &, unsigned int, Root &, Right...)
		{
			return true;
		}
		inline void reset() { }
		template <unsigned int> inline bool getCatch(CatchReturn &) const 
		{
			return false;
		}
		template <unsigned int> inline unsigned int getIdentifier() const
		{
			return 0;
		}
	};
	
	// Memory for all right context
	
	template <typename T> struct CheckMemory
	{
		static const constexpr bool have = false;
	};
	
	template <typename T> struct CheckMemory<Reference<T>>
	{
		static const constexpr bool have = CheckMemory<T>::have;
	};
	
	template <typename... Rest> struct AllRightContext;
	
	template <bool artifical> struct AllRightContext<Reference<Closure<artifical>>>
	{
		AllRightContext(Reference<Closure<artifical>>) { }
		void remember(Reference<Closure<artifical>>) { }
		void restore(Reference<Closure<artifical>>) { }
		static const constexpr bool haveMemory{false};
		std::ostream & operator>>(std::ostream & stream) const
		{
			return stream;
		}
		static std::ostream & printToStream(std::ostream & stream, Reference<Closure<artifical>>)
		{
			return stream;
		}
		static inline bool haveArtificalEnd()
		{
			return artifical;
		}
	};
	
	template <typename T, typename... Rest> struct AllRightContext<Reference<T>, Rest...>
	{
		static const constexpr bool haveMemory{CheckMemory<T>::have || AllRightContext<Rest...>::haveMemory};
		
		T objCopy;
		AllRightContext<Rest...> rest;
		AllRightContext(Reference<T> ref, Rest... irest): objCopy{ref.getRef()}, rest{irest...} { }
		void remember(Reference<T> ref, Rest... irest)
		{
			if (haveMemory || true)
			{
				objCopy = std::move(ref.getRef());
				rest.remember(irest...);
			}
		}
		void restore(Reference<T> ref, Rest... irest)
		{
			if (haveMemory || true)
			{
				ref.getRef() = std::move(objCopy);
				rest.restore(irest...);
			}
		}	
		std::ostream & operator>>(std::ostream & stream) const
		{
			objCopy.template operator>><true>(stream);
			return rest.operator>>(stream);
		}
		static std::ostream & printToStream(std::ostream & stream, Reference<T> ref, Rest... irest)
		{
			ref.getRef().template operator>><true>(stream);
			return AllRightContext<Rest...>::printToStream(stream, irest...);
		}
		static inline bool haveArtificalEnd()
		{
			return AllRightContext<Rest...>::haveArtificalEnd();
		}
	};
	
	
	template <typename T> inline Reference<T> makeRef(T & target)
	{
		return Reference<T>(target);
	}
	
	// pair representing "catched" content from input
	struct Catch
	{
		uint32_t begin;
		uint32_t length;
	};
	
	// object which represent all of "catched" pairs, support for range-based FOR
	struct CatchReturn {
	protected:
		const Catch * vdata;
		size_t vsize;
	public:
		size_t size() const {
			return vsize;
		}
		const Catch * data() const {
			return vdata;
		}
		CatchReturn(): vdata{nullptr}, vsize{0} { }
		CatchReturn(const Catch * ldata, size_t lsize): vdata{ldata}, vsize{lsize} { }
		const Catch * get(const size_t id) {
			if (id < vsize) return &vdata[id];
			else return nullptr;
		}
		const Catch * begin() const {
			return vdata;
		}
		const Catch * end() const {
			return (vdata + vsize);
		}
		const Catch operator[](unsigned int id)
		{
			if (id < vsize) return vdata[id];
			else return Catch{0,0};
		}
	};
	
	template <unsigned int id, typename T, typename... Tx> inline bool getCatchFromSubrexpHelper(CatchReturn & catches, T & from, Tx &... next)
	{
		if (!from.template getCatch<id>(catches))
		{
			return getCatchFromSubrexpHelper(catches, next...);
		}
		else
		{
			return true;
		}
	}
	
	template <unsigned int id, typename T> inline bool getCatchFromSubrexpHelper(CatchReturn & catches, T & from)
	{
		return from.template getCatch<id>(catches);
	}
	
	template <unsigned int id, typename T, typename... Tx> inline CatchReturn getCatchFromSubrexp(T & from, Tx &... next)
	{
		CatchReturn catches;
		getCatchFromSubrexpHelper<id>(catches, from, next...);
		return catches;
	}
	
	// static-allocated memory which contains "catched" pairs
	
	template <size_t size> struct StaticMemory
	{
	protected:
		uint32_t count{0};
		Catch data[size];
	public:
		inline void reset()
		{
			count = 0;
		}
		StaticMemory() = default;
		StaticMemory(const StaticMemory & right) = default;
		StaticMemory & operator=(const StaticMemory & right) = default;
		StaticMemory & operator=(StaticMemory && right)
		{
			count = right.count;
			for (unsigned int i{0}; i != count; ++i)
			{
				data[i] = right.data[i];
			}
			right.count = 0;
			return *this;
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
		size_t getCount() const
		{
			return count;
		}
		CatchReturn getCatches() const
		{
			return CatchReturn{data, count};
		}
		template <typename StringAbstraction> std::ostream & printTo(std::ostream & stream, const StringAbstraction & string) const
		{
			return stream;
		}
	};
	
	// dynamic-allocated memory (based on std::vector) which contains "catched" pairs
	
	struct DynamicMemory
	{
	protected:
		std::vector<Catch> data;
	public:
		inline void reset()
		{
			data.resize(0);
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
		size_t getCount() const
		{
			return data.size();
		}
		CatchReturn getCatches() const
		{
			return CatchReturn{data.data(), getCount()};
		}
		DynamicMemory() = default;
		DynamicMemory(const DynamicMemory & orig): data{orig.data} {
			RepeatCounter::prefix(std::cout);
			if (data.size())
			{
				std::cout << "\033[0;30;43m copy constructor \033[0m  ";
				printTo2(std::cout);
			}
			else
			{
				std::cout << "copy constructor";
			}
			std::cout << "\n";
		}
		DynamicMemory(DynamicMemory && orig): data{std::move(orig.data)}
		{
			RepeatCounter::prefix(std::cout);
			if (data.size())
			{
				std::cout << "\033[0;30;47m move constructor \033[0m  ";
				printTo2(std::cout);
			}
			else
			{
				std::cout << "move constructor";
			}
			std::cout << "\n";
		}
		DynamicMemory & operator=(const DynamicMemory & orig)
		{
			RepeatCounter::prefix(std::cout);
			if (data.size() || orig.data.size())
			{
				std::cout << "\033[1;37;43m copy operator \033[0m  ";
				printTo2(std::cout);
				std::cout << " <= ";
				orig.printTo2(std::cout);
				data = orig.data;
				std::cout << "  \033[1;37;43m                                     \033[0m  ";
			}
			else
			{
				std::cout << "copy operator";
			}
			std::cout << "\n";
			return *this;
		}
		DynamicMemory & operator=(DynamicMemory && orig)
		{
			RepeatCounter::prefix(std::cout);
			
			if (data.size() || orig.data.size())
			{
				std::cout << "\033[1;37;44m move operator \033[0m  ";
				printTo2(std::cout);
				std::cout << " <= ";
				orig.printTo2(std::cout);
				data = std::move(orig.data);
				std::cout << "  \033[1;37;44m                                     \033[0m  ";
			}
			else
			{
				std::cout << "move operator";
			}
			std::cout << "\n";
			return *this;
		}
		~DynamicMemory()
		{
			RepeatCounter::prefix(std::cout);
			if (data.size())
			{
				std::cout << "\033[0;30;47m destructor \033[0m  ";
				printTo2(std::cout);
			}
			else
			{
				std::cout << "destructor";
			}
			std::cout << "\n";
		}
		std::ostream & printTo2(std::ostream & stream) const
		{
			stream << '{';
			bool first{true};
			unsigned int id = 0;
			for (const auto & ctc: data)
			{
				if (first) first = false;
				else stream << ", ";
				stream << id++ << ": \033[1;31m" << ctc.begin << ".." << ctc.begin+ctc.length << "\033[0m len="<<ctc.length;
			}
			stream << '}';
			return stream;
		}
		template <typename StringAbstraction> std::ostream & printTo(std::ostream & stream, const StringAbstraction & string) const
		{
			stream << '{';
			bool first{true};
			unsigned int id{0};
			for (const auto & ctc: data)
			{
				if (first) first = false;
				else stream << ", ";
				stream << id++ << ": " << ctc.begin << ".." << ctc.length+ctc.length << " '" << std::string(string.original).substr(ctc.begin,ctc.length) << "' len="<<ctc.length;
			}
			stream << '}';
			return stream;
		}
	};
	
	// struct which represents Begin ^ regexp sign (matching for first-position)
	struct Begin
	{
		template <typename StringAbstraction, typename Root, typename NearestRight, typename... Right> inline bool match(const StringAbstraction string, size_t & move, unsigned int deep, Root & root, Reference<NearestRight> nright, Right... right)
		{
			if (string.isBegin())
			{
				if (nright.getRef().match(string, move, deep, root, right...))
				{
					return true;
				}
			}
			return false;
		}
		inline void reset() { }
		template <unsigned int> inline bool getCatch(CatchReturn &) const 
		{
			return false;
		}
		template <unsigned int> inline unsigned int getIdentifier() const
		{
			return 0;
		}
		template <bool> inline std::ostream & operator>>(std::ostream & stream) const
		{
			return stream << "\033[1;33m^\033[0m";
		}
	};
	
	// struct which represent End $ regexp sign (matching for end-of-input)
	struct End
	{
		template <typename StringAbstraction, typename Root, typename NearestRight, typename... Right> inline bool match(const StringAbstraction string, size_t &, unsigned int, Root &, Reference<NearestRight>, Right...)
		{
			if (string.isEnd())
			{
				// there is no need for calling rest of call-chain
				return true;
			}
			return false;
		}
		inline void reset() { }
		template <unsigned int> inline bool getCatch(CatchReturn &) const 
		{
			return false;
		}
		template <unsigned int> inline unsigned int getIdentifier() const
		{
			return 0;
		}
		template <bool> inline std::ostream & operator>>(std::ostream & stream) const
		{
			return stream << "\033[1;33m$\033[0m";
		}
	};
	
	// templated struct which represent string (sequence of characters) in regexp
	template <wchar_t firstCode, wchar_t... codes> struct String<firstCode, codes...>
	{
		template <typename StringAbstraction, typename Root, typename NearestRight, typename... Right> inline bool match(const StringAbstraction string, size_t & move, unsigned int deep, Root & root, Reference<NearestRight> nright, Right... right)
		{
			size_t pos{0};
			if (checkString(string, pos, deep))
			{
				DEBUG_PRINTF("checkString match (pos = %zu)\n",pos);
				size_t pos2{0};
				if (nright.getRef().match(string.add(pos), pos2, deep, root, right...))
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
		inline void reset() { }
		template <unsigned int> inline bool getCatch(CatchReturn &) const 
		{
			return false;
		}
		template <unsigned int> inline unsigned int getIdentifier() const
		{
			return 0;
		}
		template <bool encapsulated> inline std::ostream & operator>>(std::ostream & stream) const
		{
			//stream << "String<";
			if (encapsulated) stream << "\"";
			stream << static_cast<char>(firstCode);
			String<codes...> tmp;
			tmp.template operator>><false>(stream);
			if (encapsulated) stream << "\"";
			//stream << '>';
			return stream;
		}
	};
	
	// empty string always match if rest of callchain match
	template <> struct String<>
	{
		template <typename StringAbstraction, typename Root, typename NearestRight, typename... Right> inline bool match(const StringAbstraction string, size_t & move, unsigned int deep, Root & root, Reference<NearestRight> nright, Right... right)
		{
			return nright.getRef().match(string, move, deep, root, right...);
		}
		template <typename StringAbstraction> static inline bool checkString(const StringAbstraction, size_t &, unsigned int)
		{
			return true;
		}
		inline void reset() { }
		template <unsigned int> inline bool getCatch(CatchReturn &) const 
		{
			return false;
		}
		template <unsigned int> inline unsigned int getIdentifier() const
		{
			return 0;
		}
		template <bool encapsulated> inline std::ostream & operator>>(std::ostream & stream) const
		{
			if (encapsulated) return stream << "\033[1;33mEmpty\033[0m";
			else return stream;
		}
	};
	
	// templated struct which represent character range in regexp [a-z] or [a-zX-Y...]
	template <bool positive, wchar_t a, wchar_t b, wchar_t... rest> struct CharacterRange<positive, a, b, rest...>
	{
		static const constexpr bool isEmpty{false};
		template <typename StringAbstraction, typename Root, typename NearestRight, typename... Right> inline bool match(const StringAbstraction string, size_t & move, unsigned int deep, Root & root, Reference<NearestRight> nright, Right... right)
		{
			if ((positive && checkChar(string, deep)) || (!positive && !checkChar(string, deep) && !string.isEnd()))
			{
				size_t pos{0};
				if (nright.getRef().match(string.add(1), pos, deep, root, right...))
				{
					move = pos+1;
					return true;
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
		inline void reset() { }
		template <unsigned int> inline bool getCatch(CatchReturn &) const 
		{
			return false;
		}
		template <unsigned int> inline unsigned int getIdentifier() const
		{
			return 0;
		}
		template <bool encapsulated> inline std::ostream & operator>>(std::ostream & stream) const
		{
			if (encapsulated) { 
				stream << '[';
				if (!positive) stream << '^';
			}
			stream << static_cast<char>(a) << '-' << static_cast<char>(b);
			CharacterRange<positive, rest...> tmp;
			tmp.template operator>><false>(stream);
			if (encapsulated) stream << ']';
			return stream;
		}
	};
	
	// empty character range which represent ANY character . sign
	template <bool positive> struct CharacterRange<positive>
	{
		static const constexpr bool isEmpty{true};
		template <typename StringAbstraction, typename Root, typename NearestRight, typename... Right> inline bool match(const StringAbstraction string, size_t & move, unsigned int deep, Root & root, Reference<NearestRight> nright, Right... right)
		{
			if ((positive && checkChar(string, deep)) || (!positive && !checkChar(string, deep) && !string.isEnd()))
			{
				size_t pos{0};
				if (nright.getRef().match(string.add(1), pos, deep, root, right...))
				{
					move = pos+1;
					return true;
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
		inline void reset() { }
		template <unsigned int> inline bool getCatch(CatchReturn &) const 
		{
			return false;
		}
		template <unsigned int> inline unsigned int getIdentifier() const
		{
			return 0;
		}
		template <bool> inline std::ostream & operator>>(std::ostream & stream) const
		{
			return stream;
		}
	};	
	
	// templated struct which represents list of chars in regexp [abcdef..] or just one character
	template <bool positive, wchar_t firstCode, wchar_t... code> struct CharacterClass<positive, firstCode, code...>
	{
		static const constexpr bool isEmpty{false};
		template <typename StringAbstraction, typename Root, typename NearestRight, typename... Right> inline bool match(const StringAbstraction string, size_t & move, unsigned int deep, Root & root, Reference<NearestRight> nright, Right... right)
		{
			if ((positive && checkChar(string, deep)) || (!positive && !checkChar(string, deep) && !string.isEnd()))
			{
				size_t pos{0};
				if (nright.getRef().match(string.add(1), pos, deep, root, right...))
				{
					move = pos+1;
					return true;
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
		inline void reset() { }
		template <unsigned int> inline bool getCatch(CatchReturn &) const 
		{
			return false;
		}
		template <unsigned int> inline unsigned int getIdentifier() const
		{
			return 0;
		}
		template <bool encapsulated> inline std::ostream & operator>>(std::ostream & stream) const
		{
			if (encapsulated) { 
				stream << '[';
				if (!positive) stream << '^';
			}
			stream << static_cast<char>(firstCode);
			CharacterClass<positive, code...> tmp;
			if (!CharacterClass<positive, code...>::isEmpty) tmp.template operator>><false>(stream);
			if (encapsulated) stream << ']';
			return stream;
		}
	};
	
	// empty character represents ANY . character
	template <bool positive> struct CharacterClass<positive>
	{
		static const constexpr bool isEmpty{true};
		template <typename StringAbstraction, typename Root, typename NearestRight, typename... Right> inline bool match(const StringAbstraction string, size_t & move, unsigned int deep, Root & root, Reference<NearestRight> nright, Right... right)
		{
			if ((positive && checkChar(string, deep)) || (!positive && !checkChar(string, deep) && !string.isEnd()))
			{
				size_t pos{0};
				if (nright.getRef().match(string.add(1), pos, deep, root, right...))
				{
					move = pos+1;
					return true;
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
		inline void reset() { }
		template <unsigned int> inline bool getCatch(CatchReturn &) const 
		{
			return false;
		}
		template <unsigned int> inline unsigned int getIdentifier() const
		{
			return 0;
		}
		template <bool> inline std::ostream & operator>>(std::ostream & stream) const
		{
			return stream << "\033[1;33mAny\033[0m";
		}
	};
	
	// templated struct which represent catch-of-content braces in regexp, ID is unique identify of this content	
	template <unsigned int id, typename MemoryType, typename Inner, typename... Rest> struct CheckMemory<CatchContent<id, MemoryType, Inner, Rest...>>
	{
		static const constexpr bool have = true;
	};
	
	template <unsigned int id, typename MemoryType, typename Inner, typename... Rest> struct CatchContent<id, MemoryType, Inner, Rest...>: public CatchContent<id, MemoryType, Seq<Inner,Rest...>>
	{
		template <typename StringAbstraction, typename Root, typename NearestRight, typename... Right> inline bool match(const StringAbstraction string, size_t & move, unsigned int deep, Root & root, Reference<NearestRight> nright, Right... right)
		{
			return CatchContent<id, MemoryType, Seq<Inner,Rest...>>::match(string, move, deep, root, nright, right...);
		}
		inline void reset()
		{
			CatchContent<id, MemoryType, Seq<Inner,Rest...>>::reset();
		}
		template <unsigned int subid> inline bool getCatch(CatchReturn & catches) const
		{
			return CatchContent<id, MemoryType, Seq<Inner,Rest...>>::template getCatch<subid>(catches);
		}
		template <unsigned int key> inline unsigned int getIdentifier() const
		{
			return CatchContent<id, MemoryType, Seq<Inner,Rest...>>::template getIdentifier<key>();
		}
	};
	
	// just one inner regexp variant of catch-of-content
	template <unsigned int id, typename MemoryType> struct XMark
	{
		MemoryType & memory;
		
		uint32_t begin;
		uint32_t len{0};
		
		template <typename StringAbstraction, typename Root, typename NearestRight, typename... Right> inline bool match(const StringAbstraction string, size_t & move, unsigned int deep, Root & root, Reference<NearestRight> nright, Right... right)
		{
			// checkpoint => set length
			len = string.getPosition() - begin + 1;
			return nright.getRef().match(string, move, deep, root, right...);
			
		}
		inline void reset()
		{
			len = 0;
		}
		XMark(uint32_t lbegin, MemoryType & lmemory): memory{lmemory}, begin{lbegin}
		{
			
		}
		XMark(const XMark & orig) = default;
		template <typename StringAbstraction> void transfer(MemoryType & memory, StringAbstraction & string) const
		{
			if (len)
			{
				RepeatCounter::prefix(std::cout);
				std::stringstream ss;
				memory.printTo(ss, string);
				auto addr = memory.add({begin,len-1});
				std::string str = std::string(string.original).substr(begin,len-1);
				std::cout << "\033[1;41mXMark<"<<id<<"> catch -> " << addr << " -> " << begin << ".." << (begin+len-1) << " len="<<(len-1) << " '\033[0m"<<str<<"\033[1;41m' over ";
				std::cout << ss.str();
				std::cout<<"\033[0m\n";
			}
		}
		XMark & operator=(const XMark & orig)
		{
			begin = orig.begin;
			len = orig.len;
			return *this;
		}
		template <bool> inline std::ostream & operator>>(std::ostream & stream) const
		{
			stream << "XMark<" << id << ':' << ' ' << begin << '.' << '.';
			if (len) stream << (len-1) << '>';
			else stream << '?' << '>';
			return stream;
		}
	};
	
	template <unsigned int id, typename MemoryType, typename Inner> struct CheckMemory<CatchContent<id, MemoryType, Inner>>
	{
		static const constexpr bool have = true;
	};
	
	template <unsigned int id, typename MemoryType, typename Inner> struct CatchContent<id, MemoryType, Inner>: public Inner
	{
		MemoryType memory;
		template <typename StringAbstraction, typename Root, typename NearestRight, typename... Right> inline bool match(const StringAbstraction string, size_t & move, unsigned int deep, Root & root, Reference<NearestRight> nright, Right... right)
		{
			XMark<id, MemoryType> mark{static_cast<uint32_t>(string.getPosition()), memory};
			bool ret{Inner::match(string, move, deep, root, makeRef(mark), nright, right...)};
			if (ret)
			{
				mark.transfer(memory, string);
			}
			return ret;
		}
		inline void reset()
		{
			memory.reset();
		}
		template <unsigned int subid> inline bool getCatch(CatchReturn & catches) const
		{
			if (subid == id) 
			{
				catches = memory.getCatches();
				return true;
			}
			else return Inner::template getCatch<subid>(catches);
		}
		template <unsigned int key> inline unsigned int getIdentifier() const
		{
			return Inner::template getIdentifier<key>();
		}
		template <bool> inline std::ostream & operator>>(std::ostream & stream) const
		{
			stream << "\033[0;32mCatch<" << id << ':' << "\033[0m ";
			Inner::template operator>><!IsSequence<Inner>::value>(stream);
			stream << "\033[0;32m>\033[0m";
			return stream;
		}
	};
	
	// templated struct which represents string with content from catch (not regexp anymore :) 
	// in style: ^([a-z]+)\1$ for catching string in style "abcabc" (in catch is just "abc")
	template <unsigned int baseid, unsigned int catchid> struct ReCatch
	{
		template <typename StringAbstraction, typename Root, typename NearestRight, typename... Right> inline bool match(const StringAbstraction string, size_t & move, unsigned int deep, Root & root, Reference<NearestRight> nright, Right... right)
		{
			CatchReturn ret;
			if (root.template getCatch<baseid>(ret)) {
				//printf("catch found (size = %zu)\n",ret.size());
				const Catch * ctch{ret.get(catchid)};
				if (ctch) {
					//printf("subcatch found\n");
					for (size_t l{0}; l != ctch->length; ++l) {
						if (!string.equalToOriginal(ctch->begin+(ctch->length-l-1),l)) return false;
					}
					size_t tmp{0};
					if (nright.getRef().match(string.add(ctch->length), tmp, deep, root, right...))
					{
						move += ctch->length + tmp;
						return true;
					}
				}
			}
			return false;
		}
		inline void reset() { }
		template <unsigned int> inline bool getCatch(CatchReturn &) const 
		{
			return false;
		}
		template <unsigned int> inline unsigned int getIdentifier() const
		{
			return 0;
		}
	};
	
	template <unsigned int baseid, unsigned int catchid> struct ReCatchReverse
	{
		template <typename StringAbstraction, typename Root, typename NearestRight, typename... Right> inline bool match(const StringAbstraction string, size_t & move, unsigned int deep, Root & root, Reference<NearestRight> nright, Right... right)
		{
			CatchReturn ret;
			if (root.template getCatch<baseid>(ret)) {
				//printf("catch found (size = %zu)\n",ret.size());
				const Catch * ctch{ret.get(catchid)};
				if (ctch) {
					//printf("subcatch found\n");
					for (size_t l{0}; l != ctch->length; ++l) {
						if (!string.equalToOriginal(ctch->begin+l,l)) return false;
					}
					size_t tmp{0};
					if (nright.getRef().match(string.add(ctch->length), tmp, deep, root, right...))
					{
						move += ctch->length + tmp;
						return true;
					}
				}
			}
			return false;
		}
		inline void reset() { }
		template <unsigned int> inline bool getCatch(CatchReturn &) const 
		{
			return false;
		}
		template <unsigned int> inline unsigned int getIdentifier() const
		{
			return 0;
		}
	};
	
	// identify path thru regexp
	template <unsigned int key, unsigned int value> struct Identifier
	{
		bool matched{false};
		template <typename StringAbstraction, typename Root, typename NearestRight, typename... Right> inline bool match(const StringAbstraction string, size_t & move, unsigned int deep, Root & root, Reference<NearestRight> nright, Right... right)
		{
			if (nright.getRef().match(string, move, deep, root, right...))
			{
				matched = true;
				return true;
			}
			else 
			{
				matched = false;
				return false;
			}
		}
		inline void reset()
		{
			matched = false;
		}
		template <unsigned int> inline bool getCatch(CatchReturn &) const
		{
			return false;
		}
		template <unsigned int rkey> inline unsigned int getIdentifier() const
		{
			return matched ? (key == rkey ? value : 0) : 0;
		}
		template <bool> inline std::ostream & operator>>(std::ostream & stream) const
		{
			return stream << "Identifier<" << key << ',' << value << '>';
		}
	};
	
	// temlated struct which represent selection in regexp (a|b|c)
	template <typename FirstOption, typename... Options> struct Selection<FirstOption, Options...>: public FirstOption
	{
		Selection<Options...> rest;
		template <typename StringAbstraction, typename Root, typename NearestRight, typename... Right> inline bool match(const StringAbstraction string, size_t & move, unsigned int deep, Root & root, Reference<NearestRight> nright, Right... right)
		{
			if (FirstOption::match(string, move, deep+1, root, nright, right...))
			{
				return true;
			}
			else
			{
				//FirstOption::reset(nright, right...);
				return rest.match(string, move, deep+1, root, nright, right...);
			}
		}
		inline void reset()
		{
			FirstOption::reset();
			rest.reset();
		}
		template <unsigned int id> inline bool getCatch(CatchReturn & catches) const
		{
			return FirstOption::template getCatch<id>(catches) || rest.template getCatch<id>(catches);
		}
		template <unsigned int rkey> inline unsigned int getIdentifier() const
		{
			if (FirstOption::template getIdentifier<rkey>()) return FirstOption::template getIdentifier<rkey>();
			else return rest.template getIdentifier<rkey>();
		}
		template <bool encapsulated> inline std::ostream & operator>>(std::ostream & stream) const
		{
			if (encapsulated) stream << "\033[1;34mSel<\033[0m";
			else stream << "\033[1;34m | \033[0m";
			FirstOption::template operator>><true>(stream);
			rest.template operator>><false>(stream);
			if (encapsulated) stream << "\033[1;34m>\033[0m";
			return stream;
		}
	};
	
	// empty selection always fail
	template <> struct Selection<>
	{
		template <typename StringAbstraction, typename Root, typename NearestRight, typename... Right> inline bool match(const StringAbstraction, size_t &, unsigned int, Root &, Reference<NearestRight>, Right...)
		{
			return false;
		}
		inline void reset() {}
		template <unsigned int> inline bool getCatch(CatchReturn &) const 
		{
			return false;
		}
		template <unsigned int> inline unsigned int getIdentifier() const
		{
			return 0;
		}
		template <bool> inline std::ostream & operator>>(std::ostream & stream) const
		{
			return stream;
		}
	};
	
	// templated struct which represent sequence of another regexps 
	template <typename First, typename... Rest> struct IsSequence<Sequence<First, Rest...>> { static constexpr bool value = true; };
	
	template <typename First, typename... Rest> struct Sequence<First, Rest...>: public First
	{
		Sequence<Rest...> rest;
		template <typename StringAbstraction, typename Root, typename NearestRight, typename... Right> inline bool match(const StringAbstraction string, size_t & move, unsigned int deep, Root & root, Reference<NearestRight> nright, Right... right)
		{
			if (First::match(string, move, deep, root, makeRef(rest), nright, right...))
			{
				return true;
			}
			//First::reset(nright, right...);
			return false;
		}
		inline void reset()
		{
			First::reset();
			rest.reset();
		}
		template <unsigned int id> inline bool getCatch(CatchReturn & catches) const
		{
			return First::template getCatch<id>(catches) || rest.template getCatch<id>(catches);
		}
		template <unsigned int rkey> inline unsigned int getIdentifier() const
		{
			if (First::template getIdentifier<rkey>()) return First::template getIdentifier<rkey>();
			else return rest.template getIdentifier<rkey>();
		}
		template <bool encapsulated> inline std::ostream & operator>>(std::ostream & stream) const
		{
			if (encapsulated) stream << "\033[1;30mSeq<\033[0m";
			First::template operator>><!IsSequence<First>::value>(stream);
			stream << "\033[1;30m,\033[0m ";
			rest.template operator>><false>(stream);
			if (encapsulated) stream << "\033[1;30m>\033[0m";
			return stream;
		}
	};

	// sequence of just one inner regexp
	template <typename First> struct IsSequence<Sequence<First>> { static constexpr bool value = true; };
	
	template <typename First> struct Sequence<First>: public First
	{
		template <typename StringAbstraction, typename Root, typename NearestRight, typename... Right> inline bool match(const StringAbstraction string, size_t & move, unsigned int deep, Root & root, Reference<NearestRight> nright, Right... right)
		{
			if (First::match(string, move, deep, root, nright, right...))
			{
				return true;
			}
			//First::reset(nright, right...);
			return false;
		}
		inline void reset()
		{
			First::reset();
		}
		template <unsigned int id> inline bool getCatch(CatchReturn & catches) const
		{
			return First::template getCatch<id>(catches);
		}
		template <unsigned int rkey> inline unsigned int getIdentifier() const
		{
			return First::template getIdentifier<rkey>();
		}
		template <bool encapsulated> inline std::ostream & operator>>(std::ostream & stream) const
		{
			if (encapsulated) stream << "\033[1;30mSeq<\033[0m";
			First::template operator>><!IsSequence<First>::value>(stream);
			if (encapsulated) stream << "\033[1;30m>\033[0m";
			return stream;
		}
	};
	
	// templated struct which represents generic-loop with min,max limit
	// ()+ "plus" cycle have min 1 and min 0 (infinity)
	// ()* "star" cycle have min 0 and max 0 (infinity)
	template <unsigned int min, unsigned int max, typename Inner, typename... Rest> struct Repeat<min, max, Inner, Rest...>: public Repeat<min, max, Seq<Inner,Rest...>>
	{
		template <typename StringAbstraction, typename Root, typename NearestRight, typename... Right> inline bool match(const StringAbstraction string, size_t & move, unsigned int deep, Root & root, Reference<NearestRight> nright, Right... right)
		{
			return Repeat<min, max, Seq<Inner,Rest...>>::match(string, move, deep, root, nright, right...);
		}
		inline void reset()
		{
			Repeat<min, max, Seq<Inner,Rest...>>::reset();
		}
		template <unsigned int id> inline bool getCatch(CatchReturn & catches) const
		{
			return Repeat<min, max, Seq<Inner,Rest...>>::template getCatch<id>(catches);
		}
		template <unsigned int rkey> inline unsigned int getIdentifier() const
		{
			return Repeat<min, max, Seq<Inner,Rest...>>::template getIdentifier<rkey>();
		}
	};
	
	// cycle with just one inner regexp
	
	template <unsigned int min, unsigned int max, typename Inner> struct Repeat<min, max, Inner>: public Inner
	{
		unsigned int id{0};
		Repeat(): id{RepeatCounter::getID()}
		{
			
		}
		std::string identifyMe(const char * suffix = "") const
		{
			static char buffer[128];
			size_t len = snprintf(buffer,128,"\033[0;%02umRepeat<%u>%s\033[0m",31+id,id,suffix);
			return std::string(buffer,len);
		}
		template <typename StringAbstraction, typename Root, typename NearestRight, typename... Right> inline bool match(const StringAbstraction string, size_t & move, unsigned int deep, Root & root, Reference<NearestRight> nright, Right... right)
		{
			size_t pos{0};
			ssize_t lastFound{-1};
			Closure<true> closure;
			
			Repeat<min, max, Inner> innerContext{*this};
			AllRightContext<Reference<NearestRight>, Right...> allRightContext{nright, right...};
			
			size_t tmp;
			
			RepeatCounter::prefix(std::cout);
			std::cout << identifyMe() << " -> enter { ";
			Inner::template operator>><true>(std::cout);
			std::cout << " } . . . . . ";
			allRightContext >> std::cout;
			std::cout << "\n";
			
			RepeatCounter::enter();
			
			for (unsigned int cycle{0}; (!max) || (cycle <= max); ++cycle)
			{
				if ((cycle >= min))
				{
					RepeatCounter::prefix(std::cout);
					std::cout << identifyMe("::right") <<" -> going in...\n";
					RepeatCounter::enter();
					if (nright.getRef().match(string.add(pos), tmp = 0, deep+1, root, right...))
					{
						allRightContext.remember(nright, right...); 
						
						RepeatCounter::leave();
						RepeatCounter::prefix(std::cout);
						std::cout << ' ' << identifyMe("::right") <<" -> OK -> ";
						allRightContext >> std::cout;
						std::cout << '\n';
						
						
						//nright.getRef().reset(right...);
						lastFound = pos + tmp;
						DEBUG_PRINTF(">> found at %zu\n",lastFound);
					}
					else
					{
						RepeatCounter::leave();
						RepeatCounter::prefix(std::cout);
						std::cout << ' ' << identifyMe("::right") <<" -> fail -> ";
						allRightContext >> std::cout;
						std::cout << '\n';
						//printf("\033[1;31mfail: "); AllRightContext<Reference<NearestRight>, Right...>::visualize(nright, right...); printf("\033[0m\n");
					}
				}
				// in next expression "empty" is needed
				*this = innerContext;
				RepeatCounter::prefix(std::cout);
				std::cout << identifyMe("::inner") <<" -> going in...\n";
				RepeatCounter::enter();
				if (Inner::match(string.add(pos), tmp = 0, deep+1, root, makeRef(closure)))
				{
					RepeatCounter::leave();
					RepeatCounter::prefix(std::cout);
					std::cout << ' ' << identifyMe("::inner") <<" -> OK -> ";
					Inner::template operator>><true>(std::cout);
					std::cout << '\n';
					innerContext = *this;
					pos += tmp;
				}
				else 
				{
					RepeatCounter::leave();
					RepeatCounter::prefix(std::cout);
					std::cout << ' ' << identifyMe("::inner") <<" -> fail -> ";
					Inner::template operator>><true>(std::cout);
					std::cout << '\n';
					
					if (lastFound >= 0)
					{
						//printf("CYCLE DONE\n");
						*this = innerContext;
						
						allRightContext.restore(nright, right...);
						DEBUG_PRINTF("cycle done (cycle = %zu)\n",cycle);
						move += static_cast<size_t>(lastFound);
						RepeatCounter::leave();
						RepeatCounter::prefix(std::cout);
						
						std::cout << identifyMe() << " => TRUE";
						
						if (allRightContext.haveArtificalEnd()) std::cout << " ARTIFICAL";
						
						std::cout << "\n";
						return true;
					}
					else break;
				}
				
			}
			RepeatCounter::leave();
			RepeatCounter::prefix(std::cout);
			
			std::cout << identifyMe() << " => false\n";
			return false;
		}
		inline void reset()
		{
			Inner::reset();
		}
		template <unsigned int id> inline bool getCatch(CatchReturn & catches) const
		{
			return Inner::template getCatch<id>(catches);
		}
		template <unsigned int rkey> inline unsigned int getIdentifier() const
		{
			return Inner::template getIdentifier<rkey>();
		}
		template <bool envelope = true> std::ostream & toString(std::ostream & str) const
		{
			if (envelope)
			{
				if (min == 1 && max == 0) str << "R+<";
				else if (min == 0 && max == 0) str << "R*<";
				else str << "Rn<" << min << ',' << max << ',';
			}
			if (false) Inner::template toString<false>(str);
			else Inner::template toString<true>(str);
			if (envelope) str << ">";
			return str;
		}
		template <bool> inline std::ostream & operator>>(std::ostream & stream) const
		{
			if (min == 1 && max == 0) stream << "\033[1;35mR+("<< id <<")<\033[0m";
			else if (min == 0 && max == 0) stream << "\033[1;35mR*("<< id <<")<\033[0m";
			else stream << "\033[1;35mRn("<< id <<")<" << min << '-' << max << ':' << "\033[0m ";
			Inner::template operator>><!IsSequence<Inner>::value>(stream);
			return stream << "\033[1;35m>\033[0m";
		}
	};
	
	// wrapper for floating matching in string (begin regexp anywhere in string)
	// without Eat<...> is regexp ABC equivalent to ^ABC$
	template <typename... Inner> struct Eat: public Sequence<Inner...>
	{
		template <typename StringAbstraction, typename Root, typename NearestRight, typename... Right> inline bool match(const StringAbstraction string, size_t & move, unsigned int deep, Root & root, Reference<NearestRight> nright, Right... right)
		{
			size_t pos{0};
			
			if (!string.exists() && Sequence<Inner...>::match(string, move, deep+1, root, nright, right...))
			{
				// branch just for empty strings
				return true;
			}
			else while (string.exists(pos)) {
				size_t imove{0};
				//DEBUG_PRINTF("eating... (pos = %zu)\n",pos);
				if (Sequence<Inner...>::match(string.add(pos), imove, deep+1, root, nright, right...))
				{
					move += pos + imove;
					return true;
				}
				else
				{
					pos++;
				}
			}
			return false;
		}
		inline void reset()
		{
			Sequence<Inner...>::reset();
		}
		template <unsigned int id> inline bool getCatch(CatchReturn & catches) const
		{
			return Sequence<Inner...>::template getCatch<id>(catches);
		}
		template <unsigned int rkey> inline unsigned int getIdentifier() const
		{
			return Sequence<Inner...>::template getIdentifier<rkey>();
		}
		template <bool encapsulated> inline std::ostream & operator>>(std::ostream & stream) const
		{
			if (encapsulated) stream << "Eat<";
			Sequence<Inner...>::template operator>><false>(stream);
			if (encapsulated) stream << ">";
			return stream;
		}
	};
	
	// debug template
	template <unsigned int part, typename... Inner> struct Debug: Sequence<Inner...>
	{
		template <typename StringAbstraction, typename Root, typename NearestRight, typename... Right> inline bool match(const StringAbstraction string, size_t & move, unsigned int deep, Root & root, Reference<NearestRight> nright, Right... right)
		{
			if (Sequence<Inner...>::match(string, move, deep, root, nright, right...))
			{
				std::cout << "[part "<<part<<": match; pos="<<string.getPosition()<<"; move="<<move<<"; right="<<sizeof...(Right)<<"]\n";
				return true;
			}
			else
			{
				std::cout << "[part "<<part<<": no match; pos="<<string.getPosition()<<"; move="<<move<<"; right="<<sizeof...(Right)<<"]\n";
				return false;
			}
		}
		inline void reset()
		{
			Sequence<Inner...>::reset();
		}
		template <unsigned int id> inline bool getCatch(CatchReturn & catches) const
		{
			return Sequence<Inner...>::template getCatch<id>(catches);
		}
		template <unsigned int rkey> inline unsigned int getIdentifier() const
		{
			return Sequence<Inner...>::template getIdentifier<rkey>();
		}
	};
	
	// templated struct which contains regular expression and is used be user :)
	template <typename... Definition> struct RegularExpression
	{
		Eat<Definition...> eat;
		void reset()
		{
			eat.reset();
		}
		template <CompareFnc<char> compare = charactersAreEqual<char>> inline bool operator()(std::string string)
		{
			size_t pos{0};
			Closure<false> closure;
			return eat.match(StringAbstraction<const char *, const char, compare>(string.c_str()), pos, 0, eat, makeRef(closure));
		}
		template <CompareFnc<char> compare = charactersAreEqual<char>> inline bool operator()(const char * string)
		{
			size_t pos{0};
			Closure<false> closure;
			return eat.match(StringAbstraction<const char *, const char, compare>(string), pos, 0, eat, makeRef(closure));
		}
		template <CompareFnc<wchar_t> compare = charactersAreEqual<wchar_t>> inline bool operator()(std::wstring string)
		{
			size_t pos{0};
			Closure<false> closure;
			return eat.match(StringAbstraction<const wchar_t *, const wchar_t, compare>(string.c_str()), pos, 0, eat, makeRef(closure));
		}
		template <CompareFnc<wchar_t> compare =  charactersAreEqual<wchar_t>> inline bool operator()(const wchar_t * string)
		{
			size_t pos{0};
			Closure<false> closure;
			return eat.match(StringAbstraction<const wchar_t *, const wchar_t, compare>(string), pos, 0, eat, makeRef(closure));
		}
		template <CompareFnc<char> compare = charactersAreEqual<char>> inline bool match(std::string string)
		{
			return operator()<compare>(string);
		}
		template <CompareFnc<char> compare = charactersAreEqual<char>> inline bool match(const char * string)
		{
			return operator()<compare>(string);
		}
		template <CompareFnc<wchar_t> compare = charactersAreEqual<wchar_t>> inline bool match(std::wstring string)
		{
			return operator()<compare>(string);
		}
		template <CompareFnc<wchar_t> compare = charactersAreEqual<wchar_t>> inline bool match(const wchar_t * string)
		{
			return operator()<compare>(string);
		}
		template <unsigned int key> unsigned int getIdentifier()
		{
			return eat.template getIdentifier<key>();
		}
		template <unsigned int id> inline CatchReturn getCatch() const
		{
			CatchReturn catches;
			eat.template getCatch<id>(catches);
			return catches;
		}
		template <unsigned int id, typename StringType> inline auto part(const StringType string, unsigned int subid = 0) -> decltype(string)
		{
			return string.substr(getCatch<id>()[subid].begin, getCatch<id>()[subid].length);
		}
		template <bool encapsulated> inline std::ostream & operator>>(std::ostream & stream) const
		{
			//stream << "\033[0;32m";
			if (encapsulated) stream << "RE<";
			eat.template operator>><false>(stream);
			if (encapsulated) stream << ">";
			//stream << "\033[0m";
			return stream;
		}
	};
	
	template <typename... Definition> std::ostream & operator<<(std::ostream & stream, const RegularExpression<Definition...> & regexp)
	{
		return regexp.template operator>><false>(stream);
	}
}

#endif