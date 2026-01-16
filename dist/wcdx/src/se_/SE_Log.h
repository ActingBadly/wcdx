//******************************************SE_Log.h*******************************************
//  Enables logging to wcdx.log for debugging purposes
//  Captures all std::cout, std::cerr, std::clog output etc..
//  Used to help find sleep_hooks that the game uses.
//  This can create large log files quickly, so only enable when needed and be quick.
//*********************************************************************************************

#pragma once

//#define SE_Log 

#ifdef SE_Log
    #include <iostream>
    #include <stdexcept>
    #include <fstream>
    static std::ofstream* g_se_log_stream = nullptr;
    static std::streambuf* g_orig_cout_buf = nullptr;
    static std::streambuf* g_orig_cerr_buf = nullptr;
    static std::streambuf* g_orig_clog_buf = nullptr;

    static void Initialize_SE_Log(){
            g_se_log_stream = new std::ofstream("wcdx.log", std::ios::out);
            if (g_se_log_stream && g_se_log_stream->is_open())
            {
                g_orig_cout_buf = std::cout.rdbuf();
                g_orig_cerr_buf = std::cerr.rdbuf();
                g_orig_clog_buf = std::clog.rdbuf();
                std::cout.rdbuf(g_se_log_stream->rdbuf());
                std::cerr.rdbuf(g_se_log_stream->rdbuf());
                std::clog.rdbuf(g_se_log_stream->rdbuf());
            }
            else
            {
                delete g_se_log_stream;
                g_se_log_stream = nullptr;
            }
        }

    static void Shutdown_SE_Log(){
            if (g_se_log_stream)
            {
                // restore original buffers
                if (g_orig_cout_buf) std::cout.rdbuf(g_orig_cout_buf);
                if (g_orig_cerr_buf) std::cerr.rdbuf(g_orig_cerr_buf);
                if (g_orig_clog_buf) std::clog.rdbuf(g_orig_clog_buf);
                g_se_log_stream->close();
                delete g_se_log_stream;
                g_se_log_stream = nullptr;
                g_orig_cout_buf = g_orig_cerr_buf = g_orig_clog_buf = nullptr;
            }
        }
#endif