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
    list(string) s;
    u u, minLen;
} su;
listDeclare(su);
listDefine(su);
typedef struct {
    list(su) sus;//one other amongus reference and I skip coding
    list(string) tail;
    u applied, tMinLen, minLen;
} rule;
listDeclare(rule);
listDefine(rule);
typedef struct {
    list(var) vars;
    list(rule) rules;
} themer;

list(string) preprocess(list(string) file, bool theme) {
    for (u i = 0; i < file.len && (theme || !stringStartsWith(file.items[i], str("config:"))); i++)
        if (file.items[i].items[0] == '%') stringListRemove(&file, i--);
    return file;
}
var parseVar(string line) {
    u i = stringPos(line, ':');
    assert(i < line.len, "Missing ':' in '%s'", cptr(line));
    var res = { stringGetRange(line, 0, i), substring(line, i + 1) };
    strTrim(&res.value);
    assert(!(stringContains(res.name, '%') || stringContains(res.name, ' ') || stringContains(res.name, '\t')), "Name '%s' should not contain whitespaces or '%%'", cptr(res.name));
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
            assert(c < line.len - 1, "End of line '%s' reached before '%%'", cptr(line));
            if (line.items[c + 1] == '%')
                stringRemove(&line, c);
            else if (line.items[c + 1] == ' ' || line.items[c + 1] == '\t') {
                stringRemove(&line, c);
                while (++c < line.len && (line.items[c] == ' ' || line.items[c] == '\t'));
                assert(c < line.len && line.items[c] == '%', "Missing '%%' in  line '%s'", cptr(line));
                stringRemove(&line, c--);
            } else {
                tmp.s = split(stringGetRange(line, lc, c - lc), '\n');
                tmp.minLen = 0;
                for (u i = 0; i < tmp.s.len; i++) tmp.minLen += tmp.s.items[i].len;
                lc = ++c;
                for (; c < line.len && line.items[c] != '%'; c++);//FIXME: not again
                assert(c < line.len, "End of line '%s' reached before '%%'", cptr(line));
                var v = { stringGetRange(line, lc, c - lc), {0} };
                assert(!(stringContains(v.name, ' ') || stringContains(v.name, '\t') || stringContains(v.name, '%') || stringContains(v.name, ':')), "Variable name '%s' contains illegal characters (whitespaces, '%%' or ':')", v.name.items);
                tmp.u = varListPos(locals, v);
                if (tmp.u == locals.len) tmp.u = locals.len + varListPos(theme, v);
                assert(tmp.u < locals.len + theme.len, "No symbol with name '%s'", cptr(v.name));
                suListAdd(&res.sus, tmp);
                lc = ++c;
            }
        } else if (line.items[c] == ' ' || line.items[c] == '\t') {
            u l = c;
            while (++c < line.len && (line.items[c] == ' ' || line.items[c] == '\t'));
            stringRemoveRange(&line, l, c - l - 1);
            line.items[c = l] = '\n';
        }
    if (lc < line.len) {
        res.tail = split(stringGetRange(line, lc, line.len - lc), '\n');
        for (u i = 0; i < res.tail.len; i++) res.tMinLen += res.tail.items[i].len;
    }
    for (u i = 0; i < res.sus.len; i++) res.minLen += res.sus.items[i].minLen;
    res.minLen += res.tMinLen;
    res.applied = max(u);
    return res;
}
themer parseThemer(list(string) file, list(var) theme) {
    themer res = {0};
    if (file.len == 0) return res;
    var v = parseVar(file.items[0]);
    u i = 0;
    while (i < file.len && !stringEquals(v.name, str("config"))) {
        assert(!varListContains(res.vars, v), "A symbol with name '%s' is already specified", cptr(v.name));
        varListAdd(&res.vars, v);
        if (++i < file.len) v = parseVar(file.items[i]);
    }
    assert(stringEquals(v.name, str("config")), "Themer specifies no config");
    v.name = str("target");
    u p = varListPos(res.vars, v);
    assert(p < res.vars.len, "Themer specifies no target");
    assert(fileExists(res.vars.items[p].value), "Themer target '%s' does not exist", cptr(res.vars.items[p].value));
    while (++i < file.len)
        ruleListAdd(&res.rules, parseRule(file.items[i], res.vars, theme));
    return res;
}
u match(string line, list(string) s, u i) {
    u j = 0;
    while (i < line.len && j < s.len) {
        if (stringRangeCompare(line, i, s.items[j].len, s.items[j]) != 0) return 0;
        i += s.items[j++].len;
        if (j < s.len) for (; i < line.len && (line.items[i] == ' ' || line.items[i] == '\t'); i++);
    }
    return j < s.len ? 0 : i;
}
string apply(string line, u index, themer themer, list(var) theme) {
    for (u i = 0; i < themer.rules.len; i++) {
        if (themer.rules.items[i].applied != max(u) || themer.rules.items[i].minLen > line.len) continue;
        string res = {0};
        u lc = 0, c = 0, j = 0;
        while (j < themer.rules.items[i].sus.len && (c = match(line, themer.rules.items[i].sus.items[j].s, c)) != 0) {
            concat(&res, stringGetRange(line, lc, c - lc));
            concat(&res, themer.rules.items[i].sus.items[j].u < themer.vars.len ?
                themer.vars.items[themer.rules.items[i].sus.items[j].u].value :
                theme.items[themer.rules.items[i].sus.items[j].u - themer.vars.len].value);
            if (++j < themer.rules.items[i].sus.len)
                for (; c <= line.len - themer.rules.items[i].sus.items[j].minLen &&
                         match(line, themer.rules.items[i].sus.items[j].s, c) == 0; c++);//FIXME: every time I type this a cute little kitty dies
            else if (themer.rules.items[i].tMinLen == 0) c = line.len;
            else
                for (; c <= line.len - themer.rules.items[i].tMinLen &&
                         match(line, themer.rules.items[i].tail, c) == 0; c++);//FIXME: I'm going insane
            lc = c;
        }
        if (j == themer.rules.items[i].sus.len && c <= line.len - themer.rules.items[i].tMinLen && match(line, themer.rules.items[i].tail, c) == line.len) {
            concat(&res, stringGetRange(line, c, line.len - c));
            themer.rules.items[i].applied = index;
            return res;
        }
    }
    return line;
}
string compile(rule rule, themer themer, list(var) theme) {
    string res = {0};
    for (u i = 0; i < rule.sus.len; i++) {
        concat(&res, joinC(rule.sus.items[i].s, ' '));
        if (rule.sus.items[i].u < themer.vars.len)
            concat(&res, themer.vars.items[rule.sus.items[i].u].value);
        else concat(&res, theme.items[rule.sus.items[i].u - themer.vars.len].value);
    }
    concat(&res, joinC(rule.tail, ' '));
    return res;
}
void doit(themer themer, list(var) theme) {
    var tmp = { str("target"), {0} };
    string path = realPathR(themer.vars.items[varListPos(themer.vars, tmp)].value, str("~/.config/4theme"));
    list(string) file = split(readAllText(path), '\n');
    for (u i = file.len; i > 0 && file.items[i - 1].len == 0; i--) stringListRemove(&file, i - 1);
    list(string) out = {0};
    for (u i = 0; i < file.len; i++)
        stringListAdd(&out, apply(file.items[i], i, themer, theme));
    for (u i = 0; i < themer.rules.len; i++)
        if (themer.rules.items[i].applied == max(u)) {
            if (i == 0) {
                u j = 0; for (; j < themer.rules.len && themer.rules.items[j].applied == max(u); j++);
                if (j == themer.rules.len)
                    themer.rules.items[i].applied = out.len;
                else {
                    themer.rules.items[i].applied = themer.rules.items[j].applied;
                    for (; j < themer.rules.len; j++)
                        if (themer.rules.items[j].applied != max(u)) themer.rules.items[j].applied++;
                }
            } else {
                themer.rules.items[i].applied = themer.rules.items[i - 1].applied + 1;
                for (u j = i + 1; j < themer.rules.len; j++)
                    if (themer.rules.items[j].applied != max(u)) themer.rules.items[j].applied++;
            }
            stringListInsert(&out, compile(themer.rules.items[i], themer, theme), themer.rules.items[i].applied);
        }
    writeAllText(path, joinC(out, '\n'));
    tmp.name = str("update");
    u pos = varListPos(themer.vars, tmp);
    if (pos < themer.vars.len) system(cptr(*concat(&themer.vars.items[varListPos(themer.vars, tmp)].value, str(" &"))));
}

int main(int argc, char** argv) {
    assert(argc >= 2, "No theme provided");
    string path = str("~/.config/4theme/");
    concat3(&path, str(argv[1]), str(".theme"));
    assert(fileExists(path), "Theme '%s' not found", argv[1]);
    list(var) theme = parseTheme(preprocess(splitR(readAllText(path), '\n'), true));
    list(string) themers = listFiles(realPath(str("~/.config/4theme/")), P_REG | P_FULL);
    for (u i = 0; i < themers.len; i++)
        if (!stringEndsWith(themers.items[i], str(".themer"))) stringListRemove(&themers, i--);
    assert(themers.len > 0, "No themers found");
    for (u i = 0; i < themers.len; i++)
        doit(parseThemer(preprocess(splitR(readAllText(themers.items[i]), '\n'), false), theme), theme);
}
