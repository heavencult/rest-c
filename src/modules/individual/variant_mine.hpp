#pragma once
// ВАРИАНТ: алгоритм расчёта индивидуального пенсионного коэффициента (ИПК).
// Формула:  ИПК = min(10, annual_income / 1 000 000 * 10), округление до 2 знаков.
// Ограничение: 0 <= annual_income <= 1 000 000 000.

#include <string>
#include <cmath>
#include <cstdlib>
#include <cctype>
#include <sstream>
#include <iomanip>
#include "../../json.hpp"

namespace TaskVariant {

    const double BASE_INCOME = 1000000.0;   // доход, при котором достигается максимум
    const double MAX_IPK     = 10.0;        // лимит ИПК за один год
    const double MAX_INCOME  = 1.0e9;       // верхняя граница допустимого дохода

    struct Input {
        double      annual_income = 0.0;    // годовой доход, руб.
        std::string parse_error;
    };

    struct Output {
        double ipk           = 0.0;         // накопленный ИПК
        double annual_income = 0.0;         // доход
        double max_ipk       = MAX_IPK;     // максимум за год
        bool   limit_reached = false;       // сработал ли лимит 10 баллов
    };

    inline double round2(double x) {
        return std::floor(x * 100.0 + 0.5) / 100.0;
    }

    inline std::string fmt(double x, int digits) {
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(digits) << x;
        return ss.str();
    }

    inline Input parse(const std::string& body) {
        Input in;
        const std::string key = "\"annual_income\"";
        const std::string not_number = "annual_income must be a number";

        size_t p = body.find(key);
        if (p == std::string::npos) {
            in.parse_error = "annual_income is required";
            return in;
        }
        p = body.find(':', p + key.size());
        if (p == std::string::npos) {
            in.parse_error = not_number;
            return in;
        }
        p++;
        while (p < body.size() && std::isspace((unsigned char)body[p])) p++;

        if (p >= body.size()) { in.parse_error = not_number; return in; }
        char c = body[p];
        if (!(std::isdigit((unsigned char)c) || c == '-' || c == '+' || c == '.')) {
            in.parse_error = not_number;
            return in;
        }

        const char* start = body.c_str() + p;
        char* end = nullptr;
        double v = std::strtod(start, &end);
        if (end == start) { in.parse_error = not_number; return in; }

        for (const char* q = start; q < end; ++q) {
            if (!(std::isdigit((unsigned char)*q) || *q == '-' || *q == '+' ||
                  *q == '.' || *q == 'e' || *q == 'E')) {
                in.parse_error = not_number;
                return in;
            }
        }

        size_t after = p + (size_t)(end - start);
        while (after < body.size() && std::isspace((unsigned char)body[after])) after++;
        if (after >= body.size() || (body[after] != ',' && body[after] != '}')) {
            in.parse_error = not_number;
            return in;
        }

        in.annual_income = v;
        return in;
    }

    inline std::string validate(const Input& in) {
        if (!in.parse_error.empty())
            return in.parse_error;
        if (!std::isfinite(in.annual_income))
            return "annual_income must be a finite number";
        if (in.annual_income < 0.0 || in.annual_income > MAX_INCOME)
            return "annual_income must be in [0, 1000000000]";
        return "";
    }

    inline Output calculate(const Input& in) {
        Output out;
        out.annual_income = in.annual_income;

        double ipk = in.annual_income / BASE_INCOME * MAX_IPK;

        if (ipk > MAX_IPK) {
            ipk = MAX_IPK;
            out.limit_reached = true;
        }

        out.ipk = round2(ipk);
        return out;
    }

    inline std::string to_json(const Output& out) {
        JObj o;
        o.raw("ipk",           fmt(out.ipk, 2));
        o.raw("annual_income", fmt(out.annual_income, 2));
        o.raw("max_ipk",       fmt(out.max_ipk, 1));
        o.raw("limit_reached", out.limit_reached ? "true" : "false");
        return o.str();
    }

}
