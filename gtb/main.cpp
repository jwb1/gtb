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
#include "gtb/pch.hpp"
#include "gtb/application.hpp"
#include "gtb/exception.hpp"

int main(int argc, char* argv[])
{
    int ret_val = EXIT_FAILURE;
    try {
        ret_val = gtb::application::get()->run(argc, argv);
    }
    catch (const gtb::exception& e) {
        ret_val = gtb::handle_exception(e);
    }
    catch (const boost::exception& e) {
        ret_val = gtb::handle_exception(e);
    }
    catch (const std::exception& e) {
        ret_val = gtb::handle_exception(e);
    }
    catch (...) {
        ret_val = gtb::handle_exception();
    }

    return (ret_val);
}
