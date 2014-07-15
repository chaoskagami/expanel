{
	"info": {
		name:					"EXPanel default theme",
		author:					"chaoskagami",
		email:					"chaos.kagami@gmail.com",
		website:				"http://github.com/chaoskagami"
		version:				"0.10.0"
	},
	general: {
		placement:				"bottom",
		icon: {
			default:			"icon.png"
			width:				"24",
			height:				"24",
			default:			"#icon"
		},
		layout: {
			icons,
			taskbar,
			switcher,
			tray
		}
	},
	"widgets": {
		clock: {
			bg:					"tile.png",
			font:				"DejaVuSans-14",
			color:				"FFFFFF",
			align:				"center",
			padding:			"12",
			format:				"HH:MM:SS"
		}
		taskbar: {
			graphics: {
				active {
					bg:			"task_act.png",
					right:		"task_act_right.png",
					left:		"task_act_left.png",
					color:		"FFFFFF"
				},
				idle {
					bg:			"task_idle.png",
					right:		"task_idle_right.png",
					left:		"task_idle_left.png",
					color:		"FFFFFF"
				}
			},
			icon: {
				x:				"0",
				y:				"1",
				width:			"24",
				height:			"24"
			},
			text: {
				font:			"DejaVuSans-8",
				align:			"left",
				x:				"5",
				y:				"0"
			}
		}
		switcher: {
			graphics: {
				idle: {
					bg:			"tile.png"
				},
				active: {
					bg:			"act.png"
				},
				separate: 		"separate.png"
			},
			text: {
				padding:		"48",
				align:			"center"
			}
		}
	}
}
