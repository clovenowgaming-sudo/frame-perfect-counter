#include "Macro.hpp"

using namespace geode::prelude;

matjson::Value RecordedInput::toJson() const {
    auto obj = matjson::Value::object();
    obj["tick"] = tick;
    obj["button"] = button;
    obj["down"] = down;
    obj["isPlayer2"] = isPlayer2;
    return obj;
}

Result<RecordedInput> RecordedInput::fromJson(matjson::Value const& json) {
    RecordedInput input;
    if (!json.isObject()) {
        return Err("RecordedInput entry is not an object");
    }
    input.tick = json["tick"].asInt().unwrapOr(0);
    input.button = json["button"].asInt().unwrapOr(1);
    input.down = json["down"].asBool().unwrapOr(true);
    input.isPlayer2 = json["isPlayer2"].asBool().unwrapOr(false);
    return Ok(input);
}

void Macro::clear() {
    inputs.clear();
    levelName.clear();
    totalTicks = 0;
}

matjson::Value Macro::toJson() const {
    auto obj = matjson::Value::object();
    obj["levelName"] = levelName;
    obj["totalTicks"] = totalTicks;

    auto arr = matjson::Value::array();
    for (auto const& input : inputs) {
        arr.push(input.toJson());
    }
    obj["inputs"] = arr;
    return obj;
}

Result<Macro> Macro::fromJson(matjson::Value const& json) {
    Macro macro;
    if (!json.isObject()) {
        return Err("Macro file is not a JSON object");
    }
    macro.levelName = json["levelName"].asString().unwrapOr("");
    macro.totalTicks = json["totalTicks"].asInt().unwrapOr(0);

    if (!json.contains("inputs") || !json["inputs"].isArray()) {
        return Err("Macro file has no 'inputs' array");
    }

    for (auto const& entry : json["inputs"]) {
        auto parsed = RecordedInput::fromJson(entry);
        if (!parsed) {
            return Err("Failed to parse a recorded input: " + parsed.unwrapErr());
        }
        macro.inputs.push_back(parsed.unwrap());
    }

    return Ok(macro);
}

std::filesystem::path Macro::defaultPath() {
    return Mod::get()->getSaveDir() / "last-macro.json";
}

Result<> Macro::saveToFile(std::filesystem::path const& path) const {
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);

    auto res = utils::file::writeString(path, toJson().dump(matjson::NO_INDENTATION));
    if (!res) {
        return Err("Failed to write macro file: " + res.unwrapErr());
    }
    return Ok();
}

Result<Macro> Macro::loadFromFile(std::filesystem::path const& path) {
    auto contents = utils::file::readString(path);
    if (!contents) {
        return Err("Failed to read macro file: " + contents.unwrapErr());
    }

    auto parsed = matjson::parse(contents.unwrap());
    if (!parsed) {
        return Err("Macro file is not valid JSON");
    }

    return Macro::fromJson(parsed.unwrap());
}
