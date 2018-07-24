//  gtb: Graphics Test Bench
//  Copyright 2018 Joshua Buckman
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
#pragma once

extern "C" void OutputDebugStringA(const char* str);

namespace gtb {
    namespace error {
        // Write to Windows debug log as a stream
        class debug_stringbuf :
            public std::basic_stringbuf<char, std::char_traits<char> >
        {
        public:
            virtual ~debug_stringbuf()
            {
                sync();
            }

        protected:
            int sync()
            {
                ::OutputDebugStringA(str().c_str());
                str(std::basic_string<char>()); // Clear the string buffer

                return 0;
            }
        };

        class debug_ostream :
            public std::basic_ostream<char, std::char_traits<char> >
        {
        public:
            debug_ostream()
                : std::basic_ostream<char, std::char_traits<char> >(new debug_stringbuf())
            {}

            ~debug_ostream()
            {
                delete rdbuf();
            }
        };

        debug_ostream dbg_out;
    }
}
