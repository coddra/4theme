#include "mcx/string.h"
#include "mcx/mcx.h"
#include "mcx/file.h"
#include "mcx/assert.h"

typedef struct {
    string name;
    string value;
} var;
int varCompare(var left, var right) {
    return stringCompare(left.name, right.name);
}
listDeclareCompare(var);
listDefineCompare(var);

typedef struct {
    string s;
    u u;
} su;
listDeclare(su);
listDefine(su);
typedef struct {
    list(su) sus;//one other amongus reference and I skip coding
    string tail;
    u applied;
} rule;
listDeclare(rule);
listDefine(rule);
typedef struct {
    list(var) vars;
    list(rule) rules;
} themer;

list(string) preprocess(list(string) file, bool theme) {
    for (u i = 0; i < file.len && (theme || !stringStartsWith(file.items[i], sstr("rules:"))); i++)
        if (file.items[i].items[0] == '%') stringListRemove(&file, i--);
    return file;
}
var parseVar(string line) {
    u i = stringPos(line, ':');
    assert(i < line.len, "Missing ':' in '%s'", cptr(line));
    var res = { stringGetRange(line, 0, i), substring(line, i + 1) };
    assert(!stringContains(res.name, '%'), "Name '%s' should not contain '%%'", cptr(res.name));
    return res;
}
list(var) parseTheme(list(string) file) {
    list(var) res = {0};
    for (u i = 0; i < file.len; i++) varListAdd(&res, parseVar(file.items[i]));
    return res;
}
rule parseRule(string line, list(var) locals, list(var) theme) {
    rule res = {0};
    su tmp = {0};
    u lc = 0;
    for (u c = 0; c < line.len; c++)//FIXME: no c++
        if (line.items[c] == '%') {
            if (c < line.len - 1 && line.items[c + 1] == '%')
                stringRemove(&line, c);
            else {
                tmp.s = stringGetRange(line, lc, c - lc);
                lc = ++c;
                for (; c < line.len && line.items[c] != '%'; c++);//FIXME: not again
                assert(c < line.len, "End of line '%s' reached before '%%'", cptr(line));
                var v = { stringGetRange(line, lc, c - lc), {0} };
                tmp.u = varListPos(locals, v);
                if (tmp.u == locals.len) tmp.u = locals.len + varListPos(theme, v);
                assert(tmp.u < locals.len + theme.len, "No symbol with name '%s'", cptr(v.name));
                suListAdd(&res.sus, tmp);
                lc = ++c;
            }
        }
    if (lc < line.len) res.tail = stringGetRange(line, lc, line.len - lc);
    res.applied = max(u);
    return res;
}
themer parseThemer(list(string) file, list(var) theme) {
    themer res = {0};
    if (file.len == 0) return res;
    var v = parseVar(file.items[0]);
    u i = 0;
    while (i < file.len && !stringEquals(v.name, sstr("rules"))) {
        assert(!varListContains(res.vars, v), "A symbol with name '%s' is already specified", cptr(v.name));
        varListAdd(&res.vars, v);
        if (++i < file.len) v = parseVar(file.items[i]);
    }
    assert(stringEquals(v.name, sstr("rules")), "Themer specifies no rules");
    v.name = sstr("target");
    u p = varListPos(res.vars, v);
    assert(p < res.vars.len, "Themer specifies no target");
    assert(fileExists(res.vars.items[p].value), "Themer target '%s' does not exist", cptr(res.vars.items[p].value));
    while (++i < file.len) ruleListAdd(&res.rules, parseRule(file.items[i], res.vars, theme));
    return res;
}
string apply(string line, u index, themer themer, list(var) theme) {
    for (u i = 0; i < themer.rules.len; i++) {
        if (!stringEndsWith(line, themer.rules.items[i].tail)) continue;
        string res = {0};
        u c = 0, j = 0;
        while (j < themer.rules.items[i].sus.len && stringRangeCompare(line, c, themer.rules.items[i].sus.items[j].s.len, themer.rules.items[i].sus.items[j].s) == 0) {
            stringAddRange(&res, themer.rules.items[i].sus.items[j].s);
            if (themer.rules.items[i].sus.items[j].u < themer.vars.len)
                stringAddRange(&res, themer.vars.items[themer.rules.items[i].sus.items[j].u].value);
            else stringAddRange(&res, theme.items[themer.rules.items[i].sus.items[j].u - themer.vars.len].value);
            c += themer.rules.items[i].sus.items[j].s.len;
            if (++j < themer.rules.items[i].sus.len)
                for (; c <= line.len - themer.rules.items[i].sus.items[j].s.len &&
                       stringRangeCompare(line, c, themer.rules.items[i].sus.items[j].s.len, themer.rules.items[i].sus.items[j].s) != 0;
                     c++);//FIXME: I'm going insane
        }
        if (j == themer.rules.items[i].sus.len) {
            for (; c <= line.len - themer.rules.items[i].tail.len &&
                   stringRangeCompare(line, c, themer.rules.items[i].tail.len, themer.rules.items[i].tail) != 0;
                 c++);//FIXME: every time I type this, one little cat dies
            if (c <= line.len - themer.rules.items[i].tail.len) {
                stringAddRange(&res, themer.rules.items[i].tail);
                themer.rules.items[i].applied = index;
                return res;
            }
        }
    }
    return line;
}
string compile(rule rule, themer themer, list(var) theme) {
    string res = {0};
    for (u i = 0; i < rule.sus.len; i++) {
        stringAddRange(&res, rule.sus.items[i].s);
        if (rule.sus.items[i].u < themer.vars.len)
            stringAddRange(&res, themer.vars.items[rule.sus.items[i].u].value);
        else stringAddRange(&res, theme.items[rule.sus.items[i].u - themer.vars.len].value);
    }
    stringAddRange(&res, rule.tail);
    return res;
}
void doit(themer themer, list(var) theme) {
    var tmp = { sstr("target"), {0} };
    string path = themer.vars.items[varListPos(themer.vars, tmp)].value;
    list(string) file = split(readAllText(path), '\n');
    list(string) out = {0};
    for (u i = 0; i < file.len; i++)
        stringListAdd(&out, apply(file.items[i], i, themer, theme));
    for (u i = 0; i < themer.rules.len; i++)
        if (themer.rules.items[i].applied == max(u)) {
            themer.rules.items[i].applied = i == 0 ? out.len : themer.rules.items[i - 1].applied + 1;
            stringListInsert(&out, compile(themer.rules.items[i], themer, theme), themer.rules.items[i].applied);
        }
    writeAllText(path, joinC(out, '\n'));
}

int main(int argc, char** argv) {
    init(MCX);
    assert(argc >= 2, "No theme provided");
    string path = sstr(argv[1]);
    assert(fileExists(path), "File '%s' does not exist", argv[1]);
    list(var) theme = parseTheme(preprocess(splitR(readAllText(path), '\n'), true));
    list(string) themers = listFiles(sstr("/home/coddra/.config/4theme/"), DT_REG);
    assert(themers.len > 0, "No themer themers found");
    for (u i = 0; i < themers.len; i++)
        doit(parseThemer(preprocess(splitR(readAllText(themers.items[i]), '\n'), false), theme), theme);
}
