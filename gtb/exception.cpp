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
#include "gtb/exception.hpp"

#include <cstdlib>
#include <fstream>
#include <boost/filesystem.hpp>

namespace gtb {
    void open_exception_log_stream(std::ofstream& log_stream)
    {
        boost::filesystem::path log_file_path(std::getenv("LOCALAPPDATA"));
        log_file_path /= "gtb";
        log_file_path /= "exception.log";

        log_stream.open(log_file_path.string(), std::ios_base::out | std::ios_base::trunc);
    }

    int handle_exception(gtb::exception const& e)
    {
        std::ofstream log_stream;
        gtb::open_exception_log_stream(log_stream);
        if (log_stream.is_open()) {
            log_stream
                << "Exception caught!" << std::endl
                << "Type: gtb::exception" << std::endl
                << boost::diagnostic_information(e);
            log_stream.close();
        }
        return (EXIT_FAILURE);
    }

    int handle_exception(boost::exception const& e)
    {
        std::ofstream log_stream;
        gtb::open_exception_log_stream(log_stream);
        if (log_stream.is_open()) {
            log_stream
                << "Exception caught!" << std::endl
                << "Type: boost::exception" << std::endl
                << boost::diagnostic_information(e);
            log_stream.close();
        }
        return (EXIT_FAILURE);
    }

    int handle_exception(std::exception const& e)
    {
        std::ofstream log_stream;
        gtb::open_exception_log_stream(log_stream);
        if (log_stream.is_open()) {
            log_stream
                << "Exception caught!" << std::endl
                << "Type: std::exception" << std::endl
                << "What: " << e.what() << std::endl;
            log_stream.close();
        }
        return (EXIT_FAILURE);
    }

    int handle_exception()
    {
        std::ofstream log_stream;
        open_exception_log_stream(log_stream);
        if (log_stream.is_open()) {
            log_stream
                << "Exception caught!" << std::endl
                << "Type: *unknown*" << std::endl;
            log_stream.close();
        }
        return (EXIT_FAILURE);
    }
}
