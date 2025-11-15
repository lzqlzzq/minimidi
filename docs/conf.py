"""Sphinx configuration for the MiniMidi documentation site."""
from __future__ import annotations

from datetime import datetime
from pathlib import Path

DOCS_DIR = Path(__file__).resolve().parent
ROOT_DIR = DOCS_DIR.parent

project = "MiniMidi"
copyright = f"{datetime.now():%Y}, MiniMidi"
author = "MiniMidi Contributors"

extensions = [
    "breathe",
]

templates_path = ["_templates"]
exclude_patterns: list[str] = ["_build"]

highlight_language = "c++"
language = "en"

html_theme = "furo"

_doxygen_xml = DOCS_DIR / "_build" / "doxygen" / "xml"
breathe_projects = {
    "minimidi": str(_doxygen_xml),
}
breathe_default_project = "minimidi"
breathe_default_members = ("members",)
