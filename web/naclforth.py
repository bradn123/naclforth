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


DEFAULT_SECTION_QUOTA = 1000
DEFAULT_INDEX_QUOTA = 1000
MAX_BLOCK_SIZE = 32 * 1024
MAX_READ_COUNT = 4


class UserCounter(db.Model):
  count = db.IntegerProperty()


class UserInfo(db.Model):
  who = db.UserProperty()
  id = db.IntegerProperty()
  section_quota = db.IntegerProperty()
  index_quota = db.IntegerProperty()


class Block(db.Model):
  owner = db.IntegerProperty()
  section = db.IntegerProperty()
  index = db.IntegerProperty()
  data = db.BlobProperty()


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
      'section_quota': 0,
      'index_quota': 0
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
    uinfo.section_quota = DEFAULT_SECTION_QUOTA
    uinfo.index_quota = DEFAULT_INDEX_QUOTA
    uinfo.put()

  info = {
      'id': uinfo.id,
      'section_quota': uinfo.section_quota,
      'index_quota': uinfo.index_quota,
  }

  memcache.add(user_key, info, 60)
  
  return info


class ReadHandler(webapp.RequestHandler):
  def get(self):
    self.post()

  def post(self):
    owner = int(self.request.get('owner'))
    section = int(self.request.get('section'))
    index = int(self.request.get('index'))
    count = int(self.request.get('count', 1))

    logging.debug('reading %d,%d,%d count %d' % (owner, section, index, count))

    # Limit how much we can read at once.
    if count < 1 or count > MAX_READ_COUNT:
      logging.debug('read count out of range %d' % count)
      return

    # Only need to check permissions if not in the public section (0).
    if section != 0:
      uinfo = GetUserInfo()
      # Only you can read blocks outside the public section.
      if uinfo['id'] != owner:
        self.response.set_status(403)  # forbidden
        return

    total = ''
    for i in range(count):
      key = 'block_%d_%d_%d' % (owner, section, index + i)

      data = memcache.get(key)
      if not data:
        b = Block.get(db.Key.from_path('Block', key))
        if not b:
          logging.debug('failed read')
          self.reponse.set_status(404)  # not found
          return
        else:
          logging.debug('read from datastore')
        data = b.data
      else:
        logging.debug('read from memcache')

      total += data

    self.response.headers['Content-type'] = 'application/octet-stream'
    self.response.out.write(total)
    logging.debug('read success')


class WriteHandler(webapp.RequestHandler):
  def post(self):
    owner = int(self.request.get('owner'))
    section = int(self.request.get('section'))
    index = int(self.request.get('index'))
    data = self.request.get('data')

    # Decode from the eclectic escaping needed to make it across form data.
    data = data.replace(
        ';r', '\r').replace(
        ';n', '\n').replace(
        ';0', '\0').replace(
        ';s', ';').encode('latin1')

    # Fail if input is too big.
    if len(data) > MAX_BLOCK_SIZE:
      self.response.set_status(400)  # bad request
      return

    # Only you can write your own blocks.
    uinfo = GetUserInfo()
    # Only you can read blocks outside the public section.
    if uinfo['id'] != owner:
      self.response.set_status(403)  # forbidden
      logging.debug('owner %d not match %d' % (uinfo['id'], owner))
      return
    # It must be in range.
    if (index < 0 or index >= uinfo['index_quota'] or
        section < 0 or section >= uinfo['section_quota']):
      self.response.set_status(403)  # forbidden
      logging.debug('outside range %d %d / %d %d' % (
        index, section, uinfo['index_quota'], uinfo['section_quota']))
      return

    key = 'block_%d_%d_%d' % (owner, section, index)

    b = Block(key = db.Key.from_path('Block', key))
    b.owner = owner
    b.section = section
    b.index = index
    b.data = data
    b.put()
      
    logging.debug('wrote %d to %d,%d,%d' % (
        len(data), owner, section, index))

    memcache.set(key, data, 60)


class MainPageHandler(webapp.RequestHandler):
  def get(self):
    uinfo = GetUserInfo()
    fields = {
        'user_id': uinfo['id'],
        'section_quota': uinfo['section_quota'],
        'index_quota': uinfo['index_quota'],
        'boot_user_id': int(self.request.get('bootuserid', 0)),
        'boot_section': int(self.request.get('bootsection', 0)),
        'boot_index': int(self.request.get('bootindex', 0)),
        'boot_start': int(self.request.get('boot', 0)),
    }
    who = users.get_current_user()
    if who:
      fields['signed_in'] = True
      fields['email'] = who.email()
      fields['sign_out'] = users.create_logout_url(self.request.uri)
    else:
      fields['signed_in'] = False
      fields['sign_in'] = users.create_login_url(self.request.uri)
    path = os.path.join(os.path.dirname(__file__), 'naclforth.html')
    self.response.out.write(template.render(path, fields))


application = webapp.WSGIApplication([
    ('/', MainPageHandler),
    ('/read', ReadHandler),
    ('/write', WriteHandler),
], debug=True)


def main():
  run_wsgi_app(application)


if __name__ == "__main__":
  main()
