"Resource/ui/BaseChat.res"
{
	"ChatPanel"
	{
		"ControlName"		"EditablePanel"
		"fieldName" 		"ChatPanel"
		"visible" 		"1"
		"enabled" 		"1"
		"xpos"			"10"
		"ypos"			"240"
		"wide"	 		"300"
		"tall"	 		"210"
        "BgColor_Override" "MomGreydientStep3"
        "HistoryAlpha" "255"
	}

	"ChatHistory"
	{
		"ControlName"		"RichText"
		"fieldName"		"ChatHistory"
		"xpos"			"2"
		"ypos"			"2"
        "proportionalToParent" "1"
		"wide"	 		"f3"
		"tall"			"f30"
		"pinCorner"		"1"
		"visible"		"1"
		"enabled"		"1"
		"labelText"		""
		"textAlignment"		"south-west"
		"font"			"Default12"
		"maxchars"		"-1"
        "BgColor_Override" "MomGreydientStep4"
	}

    "ChatEntry"
    {
        "ControlName" "TextEntry"
        "fieldName" "ChatEntry"
        "visible" "0"
        "enabled" "1"
        "xpos" "0"
        "ypos" "0"
        "proportionalToParent" "1"
        "wide" "f3"
        "tall" "13" 
        "textinsetx" "2"
        "textinsety" "0"
        "font" "Default12"
        "TypingText" "White"
        "pin_to_sibling" "ChatHistory"
		"pin_corner_to_sibling"		"0"
		"pin_to_sibling_corner"		"2"
        "unicode"   "1"
        "keyboardinputenabled" "1"
        "mouseinputenabled" "1"
        "maxchars" "256"
    }

    "ChatFiltersButton"
    {
        "ControlName" "Button"
        "fieldName" "ChatFiltersButton"
        "xpos" "0"
        "ypos" "0"
        "wide" "30"
        "tall" "14"
        "pinCorner" "2"
        "visible" "1"
        "enabled" "1"
        "tabPosition" "0"
        "labelText" "#chat_filterbutton"
        "textAlignment" "center"
        "font" "Default10"
        "dulltext" "0"
        "brighttext" "0"
        "Default" "0"
        "textinsetx" "6"
        "textinsety" "0"
        "pin_to_sibling" "ChatEntry"
		"pin_corner_to_sibling"		"1"
		"pin_to_sibling_corner"		"3"
    }

    "TypingMembers"
    {
        "ControlName" "Label"
        "fieldName" "TypingMembers"
        "xpos" "-4"
        "ypos" "2"
        "visible" "0"
        "pin_to_sibling" "ChatEntry"
		"pin_corner_to_sibling"		"0"
		"pin_to_sibling_corner"		"2"
        "auto_wide_tocontents" "1"
        "paintbackground" "0"
        "font" "Default10"
        "fgcolor_override" "OffWhite"
    }
}
