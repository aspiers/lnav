/**
 * Copyright (c) 2015, Timothy Stack
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * * Neither the name of Timothy Stack nor the names of its contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __pretty_printer_hh
#define __pretty_printer_hh

#include <deque>
#include <sstream>

#include "data_scanner.hh"

class pretty_printer {

public:

    struct element {
        element(data_token_t token, pcre_context &pc)
                : e_token(token), e_capture(*pc.all()) {

        };

        data_token_t e_token;
        pcre_context::capture_t e_capture;
    };

    pretty_printer(data_scanner *ds)
            : pp_depth(0), pp_line_length(0), pp_scanner(ds) {

    };

    std::string print() {
        pcre_context_static<30> pc;
        data_token_t dt;

        while (this->pp_scanner->tokenize(pc, dt)) {
            element el(dt, pc);

            switch (dt) {
                case DT_XML_EMPTY_TAG:
                    this->start_new_line();
                    this->pp_values.push_back(el);
                    this->start_new_line();
                    continue;
                case DT_XML_OPEN_TAG:
                    this->start_new_line();
                    this->write_element(el);
                    this->pp_depth += 1;
                    continue;
                case DT_XML_CLOSE_TAG:
                    this->flush_values();
                    this->pp_depth -= 1;
                    this->write_element(el);
                    this->start_new_line();
                    continue;
                case DT_LCURLY:
                case DT_LSQUARE:
                    this->flush_values(true);
                    this->pp_values.push_back(el);
                    this->pp_depth += 1;
                    continue;
                case DT_RCURLY:
                case DT_RSQUARE:
                    this->flush_values();
                    this->pp_depth -= 1;
                    this->write_element(el);
                    continue;
                case DT_COMMA:
                    this->flush_values(true);
                    this->write_element(el);
                    this->start_new_line();
                    continue;
            }
            this->pp_values.push_back(el);
        }
        this->flush_values();
        this->pp_stream << std::endl << std::ends;

        return this->pp_stream.str();
    };

private:

    void start_new_line() {
        bool has_output;

        if (this->pp_line_length > 0) {
            this->pp_stream << std::endl;
        }
        has_output = this->flush_values();
        if (has_output) {
            this->pp_stream << std::endl;
        }
        this->pp_line_length = 0;
    }

    bool flush_values(bool start_on_depth = false) {
        bool retval = false;

        while (!this->pp_values.empty()) {
            {
                element &el = this->pp_values.front();
                this->write_element(this->pp_values.front());
                if (start_on_depth &&
                        (el.e_token == DT_LSQUARE ||
                         el.e_token == DT_LCURLY)) {
                    this->pp_stream << std::endl;
                    this->pp_line_length = 0;
                }
            }
            this->pp_values.pop_front();
            retval = true;
        }
        return retval;
    }

    void append_indent() {
        for (int lpc = 0; lpc < this->pp_depth; lpc++) {
            this->pp_stream << "    ";
        }
    }

    void write_element(const element &el) {
        if (this->pp_line_length == 0 && el.e_token == DT_WHITE) {
            return;
        }
        pcre_input &pi = this->pp_scanner->get_input();
        if (this->pp_line_length == 0) {
            this->append_indent();
        }
        this->pp_stream << pi.get_substr(&el.e_capture);
        this->pp_line_length += el.e_capture.length();
    }

    int pp_depth;
    int pp_line_length;
    data_scanner *pp_scanner;
    std::ostringstream pp_stream;
    std::deque<element> pp_values;

};

#endif
