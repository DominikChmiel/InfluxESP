import time
from aiohttp import web

"""
	Simple http server that returns the current time as milliseconds
"""

async def hello(request):
    return web.Response(text=str(int(time.time()*1000)))

app = web.Application()
app.add_routes([web.get('/', hello)])

web.run_app(app, port=8000)