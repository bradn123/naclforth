import datetime
import logging
import os
import random
import re
import sys
from google.appengine.api import memcache
from google.appengine.api import users
from google.appengine.ext import db
from google.appengine.ext import webapp
from google.appengine.ext.webapp import template
from google.appengine.ext.webapp.util import run_wsgi_app


MAX_FILE_SIZE = 1024 * 1024


class UserCounter(db.Model):
  count = db.IntegerProperty()


class UserInfo(db.Model):
  who = db.UserProperty()
  id = db.IntegerProperty()


class File(db.Model):
  owner = db.IntegerProperty()
  filename = db.StringProperty()
  data = db.TextProperty()


def AllocId():
  key = db.Key.from_path('UserCounter', 1)
  obj = UserCounter.get(key)
  if not obj:
    obj = UserCounter(key=key)
    obj.count = 0
  id = obj.count
  obj.count += 1
  obj.put()
  return id


def GetUserInfo():
  info = {
      'id': -1,
  }

  who = users.get_current_user()
  if not who: return info

  user_id = who.user_id()
  if not user_id: return info
  user_key = 'userinfo_' + user_id

  info = memcache.get(user_key)
  if info: return info

  uinfo = UserInfo.gql('where who=:1', who).get()
  if not uinfo:
    id = db.run_in_transaction(AllocId)

    uinfo = UserInfo()
    uinfo.who = who
    uinfo.id = id
    uinfo.put()

  info = {
      'id': uinfo.id,
  }

  memcache.add(user_key, info, 60)
  
  return info


class ReadHandler(webapp.RequestHandler):
  def get(self):
    self.post()

  def get(self):
    self.post()
    
  def post(self):
    owner = int(self.request.get('owner'))
    filename = self.request.get('filename')

    logging.debug('reading %d:%s' % (owner, filename))

    # Check access rights.
    if not filename.startswith('/public/'):
      uinfo = GetUserInfo()
      # Only you can read files outside /public/.
      if uinfo['id'] != owner:
        self.response.set_status(403)  # forbidden
        return

    key = 'file_%d_%s' % (owner, filename)

    data = memcache.get(key)
    if not data:
      f = File.get(db.Key.from_path('File', key))
      if not f:
        logging.debug('failed read')
        self.response.set_status(404)  # not found
        return
      else:
        logging.debug('read from datastore')
        data = f.data
    else:
      logging.debug('read from memcache')

    self.response.headers['Content-type'] = 'application/octet-stream'
    self.response.out.write(data)
    logging.debug('read success')


class WriteHandler(webapp.RequestHandler):
  def post(self):
    filename = self.request.get('filename')
    data = self.request.get('data')
                  
    # Fail if input is too big.
    if len(data) > MAX_FILE_SIZE:
      self.response.set_status(400)  # bad request
      return

    # Only you can write your own blocks.
    uinfo = GetUserInfo()
    # You can only write if logged in.
    if uinfo['id'] == -1:
      self.response.set_status(403)  # forbidden
      logging.debug('not logged in')
      return
    owner = uinfo['id']
    
    logging.debug('trying to write %d to %d:%s' % (len(data), owner, filename))

    key = 'file_%d_%s' % (owner, filename)

    f = File(key=db.Key.from_path('File', key))
    f.owner = owner
    f.filname = filename
    f.data = data
    f.put()
      
    memcache.set(key, data)
                  
    logging.debug('wrote %d to %d:%s' % (len(data), owner, filename))


class MainPageHandler(webapp.RequestHandler):
  def get(self):
    # Check that were running in Chrome of a high enough version.
    agent = self.request.headers.get('User-Agent', '')
    m = re.match('.*Chrom[^/]+\/([0-9]+)\..*', agent)
    if not m or int(m.group(1)) < 14:
      self.redirect('/getchrome')
      return
    
    boot = self.request.get('boot', '/_read?owner=0&filename=%2fpublic%2f_boot')
    uinfo = GetUserInfo()
    fields = {
        'user_id': uinfo['id'],
        'boot': boot,
    }
    who = users.get_current_user()
    if who:
      fields['signed_in'] = True
      fields['email'] = who.email()
      fields['sign_out'] = users.create_logout_url(self.request.uri)
    else:
      fields['signed_in'] = False
      fields['sign_in'] = users.create_login_url(self.request.uri)
    path = os.path.join(os.path.dirname(os.path.abspath(
          __file__)), 'templates', 'naclforth.html')
    self.response.out.write(template.render(path, fields))


class GetChromePageHandler(webapp.RequestHandler):
  def get(self):
    fields = {}
    path = os.path.join(os.path.dirname(os.path.abspath(
          __file__)), 'templates', 'getchrome.html')
    self.response.out.write(template.render(path, fields))


class GetAppPageHandler(webapp.RequestHandler):
  def get(self):
    fields = {}
    path = os.path.join(os.path.dirname(os.path.abspath(
          __file__)), 'templates', 'getapp.html')
    self.response.out.write(template.render(path, fields))


application = webapp.WSGIApplication([
    ('/', MainPageHandler),
    ('/getchrome', GetChromePageHandler),
    ('/getapp', GetAppPageHandler),
    ('/_read', ReadHandler),
    ('/_write', WriteHandler),
], debug=True)


def main():
  run_wsgi_app(application)


if __name__ == "__main__":
  main()
